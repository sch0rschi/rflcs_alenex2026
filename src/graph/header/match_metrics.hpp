#ifndef RFLCS_MATCH_METRICS_H
#define RFLCS_MATCH_METRICS_H

#include "../../instance.hpp"

using comparison_tuple = std::tuple<int, int, int, int, int>;

std::vector<int> get_single_character_repetitions(const instance &instance);

std::vector<int> get_characters_ordered_by_importance(const instance &instance);

#endif
