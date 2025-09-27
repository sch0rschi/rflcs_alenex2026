#include "header/domination_utils.hpp"
#include "../globals.hpp"
#include <ranges>
#include <algorithm>


bool dominated_by_some_available_but_unused_character(const int succ_position_2, const int depth) {
    return depth > 0
           && static_cast<int>(globals::int_vector_positions_2.size()) >= depth
           && globals::int_vector_positions_2.at(depth - 1) < succ_position_2;
}

void add_position_2_to_maybe_min_pos_2(const int succ_position_2) {
    if (const auto it = std::ranges::lower_bound(globals::int_vector_positions_2, succ_position_2);
        it == globals::int_vector_positions_2.end() || *it != succ_position_2) {
        globals::int_vector_positions_2.insert(it, succ_position_2);
    }
}
