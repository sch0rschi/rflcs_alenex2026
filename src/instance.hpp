#ifndef RFLCS_INSTANCE_HPP
#define RFLCS_INSTANCE_HPP

#include "graph/graph.hpp"
#include "mdd/mdd.hpp"
#include "mdd/shared_object.hpp"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <deque>

using unsigned_short_matrix = short*;

struct instance {
    std::string path;
    std::unique_ptr<rflcs_graph::graph> graph = nullptr;
    std::unique_ptr<struct mdd> mdd;
    std::unique_ptr<struct mdd_node_source> mdd_node_source;
    long mdd_memory_consumption = 0;
    long main_process_memory_consumption = 0;
    bool is_solving_forward = true;
    std::deque<character_type> solution = std::deque<character_type>();
    int lower_bound = 0;
    int upper_bound = 0;
    int alphabet_size = 0;
    std::vector<character_type> string_1 = std::vector<character_type>();
    unsigned_short_matrix next_occurrences_1 = unsigned_short_matrix();
    std::vector<character_type> string_2 = std::vector<character_type>();
    unsigned_short_matrix next_occurrences_2 = unsigned_short_matrix();
    int heuristic_solution_length = 0;
    std::mt19937 random = std::mt19937(0);
    bool is_valid_solution = false;
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> heuristic_solution_time;
    std::chrono::time_point<std::chrono::system_clock> heuristic_end;
    std::chrono::time_point<std::chrono::system_clock> reduction_end;
    std::chrono::time_point<std::chrono::system_clock> end;
    int active_matches = INT_MAX;
    int input_validity_code = 0;
    struct shared_object* shared_object;

    int reduction_upper_bound = 0;

    int solver_upper_bound = 0;
    int solver_solution_length = 0;
    std::chrono::time_point<std::chrono::system_clock> solver_end;
};

#endif
