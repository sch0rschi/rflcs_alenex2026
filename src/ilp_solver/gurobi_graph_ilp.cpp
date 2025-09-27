#include "ilp_solvers.h"

#include <ranges>
#include <set>
#include <cmath>

#include "gurobi_c++.h"
#include "../config.hpp"

void set_solution_from_graph(::instance &instance, const std::set<rflcs_graph::match *> &matches);

std::set<rflcs_graph::match *> get_active_matches(const instance &instance);

absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >
get_character_matches_map(const std::set<rflcs_graph::match *> &matches);

void set_objective_function(const instance &instance, GRBModel &model, const std::set<rflcs_graph::match *> &matches);

void set_repetition_free_constraint(GRBModel &model, const std::set<rflcs_graph::match *> &matches);

void set_common_sub_sequence_constraint(GRBModel &model, const std::set<rflcs_graph::match *> &matches);

void solve_gurobi_graph_ilp(instance &instance) {
    try {
        GRBEnv env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, SOLVER_TIMEOUT_IN_SECONDS);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        // TODO: benchmark
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve
        // TODO: benchmark
        //env.set(GRB_IntParam_Cuts, 0);
        // TODO: benchmark
        env.set(GRB_IntParam_ScaleFlag, 3);
        //        env.set(GRB_IntParam_VarBranch, GRB_VARBRANCH_MAX_INFEAS);

        GRBModel model = GRBModel(env);

        auto matches = get_active_matches(instance);

        set_objective_function(instance, model, matches);
        set_common_sub_sequence_constraint(model, matches);
        set_repetition_free_constraint(model, matches);
        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            instance.lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            instance.solver_upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
            set_solution_from_graph(instance, matches);
        } else if (result_status == GRB_INFEASIBLE) {
            instance.solver_upper_bound = instance.lower_bound;
        } else {
            instance.solver_upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
        }
    } catch (GRBException &e) {
        std::cout << "Error code = " << e.getErrorCode() << std::endl;
        std::cout << e.getMessage() << std::endl;
    }
}

void set_common_sub_sequence_constraint(GRBModel &model, const std::set<rflcs_graph::match *> &matches) {
    for (const auto match1: matches) {
        for (const auto match2: matches) {
            if (match1->extension.position_1 < match2->extension.position_1
                && match1->extension.position_2 > match2->extension.position_2) {
                auto conflict = GRBLinExpr();
                conflict += match1->extension.gurobi_variable;
                conflict += match2->extension.gurobi_variable;
                model.addConstr(conflict, GRB_LESS_EQUAL, 1.0);
            }
        }
    }
}

void set_repetition_free_constraint(GRBModel &model, const std::set<rflcs_graph::match *> &matches) {
    for (const auto &[character, matches_with_character]: get_character_matches_map(matches)) {
        auto same_character_sum = GRBLinExpr();
        for (const auto same_character_match: matches_with_character) {
            same_character_sum += same_character_match->extension.gurobi_variable;
        }
        model.addConstr(same_character_sum, GRB_LESS_EQUAL, 1.0);
    }
}

void set_objective_function(const instance &instance, GRBModel &model, const std::set<rflcs_graph::match *> &matches) {
    auto objective = GRBLinExpr();
    for (const auto match: matches) {
        match->extension.gurobi_variable = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        objective += match->extension.gurobi_variable;
    }
    model.setObjective(objective, GRB_MAXIMIZE);
    model.addConstr(objective, GRB_GREATER_EQUAL, instance.lower_bound + 1);
}

absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >
get_character_matches_map(const std::set<rflcs_graph::match *> &matches) {
    auto character_matches_map = absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >();
    for (const auto match: matches) {
        if (!character_matches_map.contains(match->character)) {
            character_matches_map.insert({match->character, std::vector<rflcs_graph::match *>()});
        }
        character_matches_map[match->character].push_back(match);
    }
    return character_matches_map;
}

std::set<rflcs_graph::match *> get_active_matches(const instance &instance) {
    std::set<rflcs_graph::match *> matches = std::set<rflcs_graph::match *>();
    for (auto &match: instance.graph->matches | std::ranges::views::drop(1) |
                      std::ranges::views::take(instance.graph->matches.size() - 2)) {
        if (match.extension.is_active) {
            matches.insert(&match);
        }
    }
    return matches;
}

void set_solution_from_graph(::instance &instance, const std::set<rflcs_graph::match *> &matches) {
    auto matches_in_solution = std::vector<rflcs_graph::match *>();
    std::ranges::copy_if(matches, std::back_inserter(matches_in_solution), [](const rflcs_graph::match *match) {
        return static_cast<int>(round(match->extension.gurobi_variable.get(GRB_DoubleAttr_X))) == 1;
    });

    std::ranges::sort(matches_in_solution, [](const rflcs_graph::match *m1, const rflcs_graph::match *m2) {
        return m1->extension.position_1 < m2->extension.position_1;
    });

    instance.solution.clear();
    for (const auto match_in_solution: matches_in_solution) {
        instance.solution.push_back(match_in_solution->character);
    }
}
