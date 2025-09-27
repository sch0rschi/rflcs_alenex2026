#include "header/character_bound_utils.hpp"

#include "../globals.hpp"

bool are_enough_characters_available(const int lower_bound,
                                     const int depth,
                                     const boost::dynamic_bitset<>& pred_available_characters,
                                     const boost::dynamic_bitset<>& succ_available_characters) {
    const auto current_size = pred_available_characters.count();
    globals::temp_character_set_1 = pred_available_characters;
    globals::temp_character_set_1 &= succ_available_characters;
    const auto intersect_size = globals::temp_character_set_1.count();
    const auto optimal_character_usage_size = current_size - intersect_size;
    const auto minimal_overlap = std::max(0, static_cast<int>(depth - optimal_character_usage_size));

    const auto potential_size = succ_available_characters.count();
    return static_cast<int>(potential_size - minimal_overlap) > lower_bound - depth;
}
