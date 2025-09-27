#include "config.hpp"
#include "globals.hpp"
#include "graph/header/graph_creation.hpp"
#include "graph/header/graph_writer.h"
#include "heuristic.hpp"
#include "ilp_solver/ilp_solvers.h"
#include "instance.hpp"
#include "reduction_orchestration.hpp"
#include "result_writer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <set>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <memory>
#include <sys/resource.h>

#include "utils.h"

auto parse_next_integer(std::ifstream &input_file) -> int;

void parse_string(std::vector<character_type> &character_sequence, std::ifstream &input_file, int string_length);

void check_solution(instance &instance);

void heuristic(instance &instance);

void reduction(instance &instance);

void solve(instance &instance);

void process_input(instance &instance, int argc, char **argv);

void solve_enumeration(instance &instance);

auto main(const int argc, char **argv) -> int {
    try {
        const std::unique_ptr<instance> instance = std::make_unique<struct instance>();

        process_input(*instance, argc, argv);

        if (instance->input_validity_code != 0) {
            return instance->input_validity_code;
        }

        globals::alphabet_size = instance->alphabet_size;
        globals::temp_character_set_1 = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::temp_character_set_2 = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::old_characters_on_paths_to_some_sink = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::old_characters_on_all_paths_to_lower_bound_levels = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::old_characters_on_paths_to_root = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::old_characters_on_all_paths_to_root = boost::dynamic_bitset<>(instance->alphabet_size);
        globals::chaining_numbers = std::vector<int>(instance->alphabet_size);
        globals::node_character_count = std::vector<long>(instance->alphabet_size);
        globals::ingoing_arc_character_count = std::vector<long>(instance->alphabet_size);
        globals::outgoing_arc_character_count = std::vector<long>(instance->alphabet_size);
        globals::int_vector_positions_2 = std::vector<int>();

        instance->start = std::chrono::system_clock::now();
        create_graph(*instance);
        if (IS_WRITING_DOT_FILE) {
            write_graph_dot(instance->graph->matches, instance->string_1, instance->string_2, "graph.dot", false);
            write_graph_dot(instance->graph->reverse_matches, instance->string_1, instance->string_2,
                            "graph_reverse.dot", true);
        }

        heuristic(*instance);
        instance->heuristic_solution_length = instance->lower_bound;
        instance->heuristic_end = std::chrono::system_clock::now();
        const std::chrono::duration<double> heuristic_elapsed_seconds = instance->heuristic_end - instance->start;
        std::cout << std::fixed << std::setprecision(2)
                << "Heuristic finished in " << heuristic_elapsed_seconds.count() << "s." << std::endl;

        instance->mdd_node_source = std::make_unique<mdd_node_source>();

        reduction(*instance);
        instance->reduction_end = std::chrono::system_clock::now();
        instance->reduction_upper_bound = instance->upper_bound;

        solve(*instance);
        instance->end = std::chrono::system_clock::now();
        instance->upper_bound = std::max(instance->upper_bound, instance->lower_bound);

        check_solution(*instance);
        write_result_file(*instance);

        const std::chrono::duration<double> solve_elapsed_seconds = instance->end - instance->reduction_end;
        const std::chrono::duration<double> elapsed_seconds = instance->end - instance->start;
        std::cout << std::fixed << std::setprecision(2)
                << solve_elapsed_seconds.count() << "\t"
                << instance->lower_bound << "\t"
                << (instance->is_valid_solution ? "true" : "false") << "\t"
                << elapsed_seconds.count() << "\t"
                << "found solution: [";
        for (const auto character: instance->solution) {
            std::cout << character << ", ";
        }
        std::cout << "]" << std::endl;

        delete instance->next_occurrences_1;
        delete instance->next_occurrences_2;

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

void process_input(instance &instance, const int argc, char **argv) {
    std::string path;
    switch (getopt(argc, argv, "i:")) {
        case 'i':
            path = optarg;
            break;
        case -1:
            path = default_path;
            break;
        default:
            path = "";
            break;
    }

    std::ifstream input_file(path);
    instance.path = path;

    std::cout << "solving file: " << instance.path << std::endl;
    std::flush(std::cout);

    if (input_file.is_open()) {
        if (int const number_of_strings = parse_next_integer(input_file); 2 != number_of_strings) {
            std::cout << "Only instances with two strings are allowed." << std::endl;
            instance.input_validity_code = 1;
        } else {
            instance.alphabet_size = parse_next_integer(input_file);
            instance.lower_bound = 0;
            instance.upper_bound = instance.alphabet_size;
            const auto string_1_length = parse_next_integer(input_file);
            parse_string(instance.string_1, input_file, string_1_length);
            const auto string_2_length = parse_next_integer(input_file);
            parse_string(instance.string_2, input_file, string_2_length);
        }
    } else {
        std::cout << "Cannot find input file: " << path << "." << std::endl;
        instance.input_validity_code = 1;
    }

    input_file.close();
}

void heuristic(instance &instance) {
    std::cout << "Heuristic started." << std::endl;
    std::flush(std::cout);
    heuristic_solve(instance);
}

void reduction(instance &instance) {
    if (instance.lower_bound >= instance.upper_bound) {
        instance.upper_bound = instance.lower_bound;
        instance.active_matches = 0;
        return;
    }
    std::cout << "reduction is running." << std::endl;
    reduce_graph_pre_solver(instance);
    instance.upper_bound = std::max(instance.upper_bound, instance.lower_bound);
}

void solve(instance &instance) {
    if (instance.lower_bound >= instance.upper_bound) {
        instance.solver_end = std::chrono::system_clock::now();
        instance.end = std::chrono::system_clock::now();
        instance.active_matches = 0;
        instance.is_valid_solution = true;
        return;
    }

    std::cout << "Solver is running." << std::endl;

    if (SOLVER == ENUMERATION || instance.shared_object->is_mdd_reduction_complete) {
        solve_enumeration(instance);
        if (!instance.is_solving_forward) {
            std::ranges::reverse(instance.solution);
        }
    } else if constexpr (SOLVER == GUROBI_GRAPH) {
        instance.upper_bound = instance.reduction_upper_bound;
        instance.lower_bound = instance.heuristic_solution_length;
        solve_gurobi_graph_ilp(instance);
        instance.solver_upper_bound = std::min(instance.solver_upper_bound, instance.upper_bound);
    }
    instance.solver_end = std::chrono::system_clock::now();
    instance.solver_solution_length = instance.lower_bound;
    instance.upper_bound = std::min(instance.upper_bound, instance.solver_upper_bound);

    if (int status; WIFEXITED(status)) {
        auto usage = rusage();
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            instance.main_process_memory_consumption = sanitize_memory_usage(usage.ru_maxrss);
        } else {
            instance.main_process_memory_consumption = -1;
        }
    } else {
        instance.main_process_memory_consumption = -1;
    }
}

void check_solution(instance &instance) {
    auto characters = std::set<int>();
    int position_1 = 0;
    int position_2 = 0;
    if (!instance.is_valid_solution) {
        std::cout << "Exact solver did not terminate.\t";
    }
    for (const auto character: instance.solution) {
        if (position_1 >= static_cast<int>(instance.string_1.size()) ||
            position_2 >= static_cast<int>(instance.string_2.size())) {
            std::cout << "String is out of available_characters.\t";
            instance.is_valid_solution = false;
            return;
        }
        if (characters.contains(character)) {
            std::cout << "Character " << character << " already in use.\t";
            instance.is_valid_solution = false;
            return;
        }
        characters.insert(character);
        position_1 = instance.next_occurrences_1[position_1 * instance.alphabet_size + character];
        position_2 = instance.next_occurrences_2[position_2 * instance.alphabet_size + character];
    }
    if (static_cast<int>(characters.size()) != instance.lower_bound) {
        std::cout << "Solution lower bound " << instance.lower_bound << " does not fit solution length of "
                << characters.size() << ".\t";
        instance.is_valid_solution = false;
        return;
    }
    if (position_1 > static_cast<int>(instance.string_1.size()) ||
        position_2 > static_cast<int>(instance.string_2.size())) {
        std::cout << "Strings are out of characters.\t";
        instance.is_valid_solution = false;
    }
}

void parse_string(std::vector<character_type> &character_sequence, std::ifstream &input_file, const int string_length) {
    character_sequence.resize(string_length);
    for (int i = 0; i < string_length && input_file.good(); i++) {
        character_sequence.at(i) = static_cast<character_type>(parse_next_integer(input_file));
    }
}

auto parse_next_integer(std::ifstream &input_file) -> int {
    if (input_file.good()) {
        std::string file_entry; // NOLINT(*-const-correctness)
        input_file >> file_entry;
        return static_cast<character_type>(std::stoi(file_entry));
    }
    return -1;
}
