#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string_view>

enum Solver {
    GUROBI_GRAPH,
    ENUMERATION,
};

constexpr std::string_view default_path = "../RFLCS_instances/type1/512_8reps.24";
constexpr long MDD_TIMEOUT_IN_SECONDS = 1800;
constexpr int SOLVER_TIMEOUT_IN_SECONDS = 1800;
constexpr bool IS_WRITING_DOT_FILE = false;
constexpr bool IS_WRITING_TIME_SERIES = false;
constexpr Solver SOLVER = GUROBI_GRAPH;
constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;

#endif
