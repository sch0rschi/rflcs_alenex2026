#include "header/simple_upper_bounds.hpp"
#include "match_loop_utils.h"

#include <memory>
#include <ranges>

bool set_simple_upper_bounds(std::vector<rflcs_graph::match> &matches);

bool calculate_simple_upper_bounds(instance &instance) {
    auto is_improving = set_simple_upper_bounds(instance.graph->matches);
    is_improving |= set_simple_upper_bounds(instance.graph->reverse_matches);

    int upper_bound = 0;
    for (const auto *dominating_match: instance.graph->matches.front().dom_succ_matches) {
        if (dominating_match->extension.is_active) {
            upper_bound = std::max(upper_bound, dominating_match->upper_bound);
        }
    }
    int reverse_upper_bound = 0;
    for (const auto *dominating_match: instance.graph->reverse_matches.front().dom_succ_matches) {
        if (dominating_match->extension.is_active) {
            reverse_upper_bound = std::max(reverse_upper_bound, dominating_match->upper_bound);
        }
    }
    instance.upper_bound = std::min(upper_bound, reverse_upper_bound);
    return is_improving;
}


void setup_matches_for_simple_upper_bound(const instance &instance) {
    instance.graph->matches.back().upper_bound = 0;
    instance.graph->reverse_matches.back().upper_bound = 0;
    for (auto &[character, upper_bound, _dom, _heur, extension]: instance.graph->matches) {
        extension.available_characters = boost::dynamic_bitset<>(instance.alphabet_size);
        extension.reversed->extension.available_characters = boost::dynamic_bitset<>(instance.alphabet_size);
    }
}

bool set_simple_upper_bounds(std::vector<rflcs_graph::match> &matches) {
    auto is_improving = false;
    for (auto &[character, upper_bound, dom_succ_matches, _heur, extension]: matches
                                                                             | std::views::reverse
                                                                             | active_match_filter) {
        auto max_succ_upper_bound = -1; // 1 gets added (=0) to root node
        if (character != SHRT_MAX) {
            extension.available_characters.reset();
            extension.available_characters.set(character);
        }
        for (const auto *potential_match: dom_succ_matches) {
            if (potential_match->extension.is_active) {
                extension.available_characters |= potential_match->extension.available_characters;
                max_succ_upper_bound = std::max(max_succ_upper_bound, potential_match->upper_bound);
            }
        }
        const auto previous_upper_bound = upper_bound;
        upper_bound = std::min({
                                       upper_bound,
                                       max_succ_upper_bound + 1,
                                       static_cast<int>(extension.available_characters.count())
                               });
        is_improving |= upper_bound < previous_upper_bound;

    }
    for (const auto *potential_match: matches.front().dom_succ_matches) {
        if (potential_match->extension.is_active) {
            matches.front().extension.available_characters |= potential_match->extension.available_characters;
        }
    }
    return is_improving;
}
