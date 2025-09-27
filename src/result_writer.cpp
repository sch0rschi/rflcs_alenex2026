#include "result_writer.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

double calculate_reduction(const instance &instance);

void replace_instances_with_results_folder(std::string &str, const std::string &from, const std::string &to) {
    if (const size_t start_pos = str.find(from); start_pos != std::string::npos) {
        str.replace(start_pos, from.length(), to);
    }
}

void write_result_file(const instance &instance) {
    std::string file_name = instance.path;

    replace_instances_with_results_folder(file_name, "RFLCS_instances", "results");
    file_name += ".out";

    std::filesystem::path path(file_name);
    std::filesystem::path dir = path.parent_path();

    if (!dir.empty() && !exists(dir)) {
        create_directories(dir);
    }

    std::ofstream out_file(file_name);
    if (!out_file) {
        std::cerr << "Failed to open file for writing." << std::endl;
    }

    std::chrono::duration<double> overall_runtime = instance.end - instance.start;
    std::chrono::duration<double> heuristic_solution_runtime = instance.heuristic_solution_time - instance.start;
    std::chrono::duration<double> heuristic_runtime = instance.heuristic_end - instance.start;
    std::chrono::duration<double> mdd_runtime = instance.reduction_end - instance.heuristic_end;
    std::chrono::duration<double> solver_runtime = instance.end - instance.reduction_end;

    out_file << "solved:\t" << std::boolalpha << instance.is_valid_solution << std::endl;
    out_file << "solution_length:\t" << instance.lower_bound << std::endl;
    out_file << "upper_bound:\t" << instance.upper_bound << std::endl;
    out_file << "solution_runtime:\t" << overall_runtime.count() << std::endl;

    out_file << "heuristic_solution_length:\t" << instance.heuristic_solution_length << std::endl;
    out_file << "heuristic_solution_runtime:\t" << heuristic_solution_runtime.count() << std::endl;
    out_file << "heuristic_runtime:\t" << heuristic_runtime.count() << std::endl;

    out_file << "solving_forward:\t" << instance.is_solving_forward << std::endl;

    out_file << "reduction_is_complete:\t" << (instance.shared_object == nullptr
        || instance.shared_object->is_mdd_reduction_complete) << std::endl;
    out_file << "reduction_quality:\t" << calculate_reduction(instance) << std::endl;
    out_file << "reduction_upper_bound:\t" << instance.reduction_upper_bound << std::endl;
    out_file << "reduction_runtime:\t" << mdd_runtime.count() << std::endl;
    out_file << "mdd_memory_consumption:\t" << instance.mdd_memory_consumption << std::endl;

    out_file << "main_process_memory_consumption:\t" << instance.main_process_memory_consumption << std::endl;

    out_file << "solver_solution_length:\t" << instance.solver_solution_length << std::endl;
    out_file << "solver_upper_bound:\t" << instance.solver_upper_bound << std::endl;
    out_file << "solver_runtime:\t" << solver_runtime.count() << std::endl;

    out_file << "solution:\t[";
    for (const auto character: instance.solution) {
        out_file << character << ", ";
    }
    out_file << "]" << std::endl;
    out_file.close();
}

double calculate_reduction(const instance &instance) {
    const double active_ratio = static_cast<double>(instance.active_matches)
                          / (static_cast<double>(instance.graph->matches.size()) - 2.0);
    return 1 - active_ratio;
}
