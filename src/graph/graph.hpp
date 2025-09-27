#ifndef RFLCS_GRAPH_H
#define RFLCS_GRAPH_H

#include <climits>
#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "gurobi_c++.h"

typedef short character_type;

namespace rflcs_graph {

    struct match;

    struct match_extension {
        bool is_active = true;
        int match_id;
        match* reversed;
        int combined_upper_bound = INT_MAX;
        std::vector<match *> succ_matches = std::vector<match *>();
        std::vector<int> rf_relaxed_upper_bounds = std::vector<int>();
        match* heuristic_previous_match;
        boost::dynamic_bitset<> available_characters;
        std::vector<int> repetition_counter;
        int position_1;
        int position_2;
        int transient_match_domination_number;
        long double search_space_occurrence_estimation = 1.0;
        GRBVar gurobi_variable;
        int lcs_depth = 0;
    };

    struct match {
        character_type character = -1;
        int upper_bound = INT_MAX; // including this character
        std::vector<match *> dom_succ_matches = std::vector<match *>();
        boost::dynamic_bitset<> heuristic_characters;
        match_extension extension;
    };

    struct graph {
        std::vector<match> matches = std::vector<match>();
        std::vector<match> reverse_matches = std::vector<match>();
    };

    inline auto position_1_comparator(const match* first, const match* second) -> bool {
        return first->extension.position_1 < second->extension.position_1;
    }
}

#endif
