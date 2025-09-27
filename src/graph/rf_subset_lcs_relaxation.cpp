#include "../instance.hpp"
#include "header/rf_subset_lcs_relaxation.hpp"

#include "header/match_metrics.hpp"
#include "graph.hpp"

#include <boost/dynamic_bitset.hpp>
#include <climits>
#include <cmath>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <ranges>

auto set_rf_relaxed_upper_bounds(std::vector<rflcs_graph::match>& matches,
                                 const std::vector<std::vector<std::pair<int, int>>>& contexts,
                                 int number_of_characters) -> bool;

auto all_single_character_relaxation(instance& instance, std::vector<std::vector<std::pair<int, int>>>& upper_bound_redistributions) -> bool;

auto selected_characters_rf_relaxation(instance& instance, std::vector<std::vector<std::pair<int, int>>>& upper_bound_redistributions) -> bool;

auto relax_by_fixed_character_rf_constraint(instance& instance) -> bool {
    auto upper_bound_redistributions = std::vector<std::vector<std::pair<int, int>>>(instance.alphabet_size);
    auto found_improvement = all_single_character_relaxation(instance, upper_bound_redistributions);
    found_improvement |= selected_characters_rf_relaxation(instance, upper_bound_redistributions);
    return found_improvement;
}

inline auto selected_characters_rf_relaxation(instance& instance, std::vector<std::vector<std::pair<int, int>>>& upper_bound_redistributions) -> bool {
    auto found_improvement = false;
    for (int number_of_selected_characters = 2;
         number_of_selected_characters < std::min(instance.alphabet_size, static_cast<int>(log2(instance.alphabet_size)));
         number_of_selected_characters++) {
        auto still_improving = true;
        while (still_improving) {
            std::vector<int> relaxation_characters = get_characters_ordered_by_importance(instance);
            relaxation_characters.resize(
                std::min(number_of_selected_characters, static_cast<int>(relaxation_characters.size())));

            for (int relaxation_character_index = 0;
                 relaxation_character_index <
                 std::min(number_of_selected_characters, static_cast<int>(relaxation_characters.size()));
                 relaxation_character_index++) {
                const auto relaxation_character = relaxation_characters.at(relaxation_character_index);
                for (int encoded = 0; encoded < 1 << number_of_selected_characters; encoded++) {
                    auto encoded_set = boost::dynamic_bitset<>(0);
                    encoded_set.append(encoded);
                    if (!encoded_set.test(relaxation_character_index)) {
                        auto without = encoded_set.to_ulong();
                        encoded_set.set(relaxation_character_index);
                        auto with = encoded_set.to_ulong();
                        upper_bound_redistributions.at(relaxation_character).emplace_back(without, with);
                    }
                }
            }


            still_improving = set_rf_relaxed_upper_bounds(instance.graph->matches, upper_bound_redistributions, number_of_selected_characters);
            still_improving |= set_rf_relaxed_upper_bounds(instance.graph->reverse_matches, upper_bound_redistributions, number_of_selected_characters);
            found_improvement |= still_improving;
            for (const auto character: relaxation_characters) {
                upper_bound_redistributions.at(character).clear();
            }
        }
    }
    return found_improvement;
}

inline auto all_single_character_relaxation(instance& instance, std::vector<std::vector<std::pair<int, int>>>& upper_bound_redistributions) -> bool {
    auto found_improvement = false;
    auto still_improving = true;
    const std::vector<int> single_character_repetitions = get_single_character_repetitions(instance);
    while (still_improving) {
        still_improving = false;
        for (int character = 0; character < instance.alphabet_size; character++) {
            if (single_character_repetitions.at(character) > 1) {
                upper_bound_redistributions.at(character).emplace_back(0, 1);
                still_improving |= set_rf_relaxed_upper_bounds(instance.graph->matches, upper_bound_redistributions, 1);
                still_improving |= set_rf_relaxed_upper_bounds(instance.graph->reverse_matches, upper_bound_redistributions, 1);
                found_improvement |= still_improving;

                upper_bound_redistributions.at(character).clear();
            }
        }
    }
    return found_improvement;
}

inline auto set_rf_relaxed_upper_bounds(std::vector<rflcs_graph::match>& matches,
                                        const std::vector<std::vector<std::pair<int, int>>>& contexts,
                                        const int number_of_characters) -> bool {
    const auto width = 1 << number_of_characters;

    auto& reverse_root_rf_relaxed_upper_bounds = matches.back().extension.rf_relaxed_upper_bounds;
    reverse_root_rf_relaxed_upper_bounds.resize(width);
    std::ranges::fill(reverse_root_rf_relaxed_upper_bounds, INT_MIN);
    reverse_root_rf_relaxed_upper_bounds.front() = 0;

    auto improved = false;

    for (auto& [character, upper_bound, dom_succ_matches, _heur, extension]: matches | std::views::reverse | std::views::drop(1) | std::views::take(matches.size() - 2)) {
        if (extension.is_active) {
            extension.available_characters.reset();
            extension.available_characters.set(character);

            // setup rf relaxed bounds
            auto& current_upper_bounds = extension.rf_relaxed_upper_bounds;
            current_upper_bounds.resize(width);
            std::ranges::fill(current_upper_bounds, 0);

            // max from predecessors upper_bounds
            for (const auto* potential_match: dom_succ_matches) {
                if (potential_match->extension.is_active) {
                    extension.available_characters |= potential_match->extension.available_characters;
                    transform(current_upper_bounds.begin(),
                              current_upper_bounds.end(),
                              potential_match->extension.rf_relaxed_upper_bounds.begin(),
                              current_upper_bounds.begin(),
                              [](auto current, auto potential) -> auto {
                                  return std::max(current, potential);
                              });
                }
            }

            // add the step
            std::ranges::transform(current_upper_bounds,
                                   current_upper_bounds.begin(),
                                   [](auto value) -> auto { return value + 1; });

            // shift and decrement if context
            for (const auto& [index_without_character, index_with_character]: contexts.at(character)) {
                current_upper_bounds.at(index_with_character) = std::max(
                    current_upper_bounds.at(index_without_character),
                    current_upper_bounds.at(index_with_character) - 1);
                current_upper_bounds.at(index_without_character) -= 1;
            }

            // upper_bound is monotonic decreasing
            std::ranges::transform(extension.rf_relaxed_upper_bounds,
                                   extension.rf_relaxed_upper_bounds.begin(),
                                   [upper_bound](const auto relaxed_upper_bound) -> int {
                                       return std::min(upper_bound, relaxed_upper_bound);
                                   });

            const auto previous_upper_bound = upper_bound;
            upper_bound = *std::ranges::max_element(current_upper_bounds);
            upper_bound = std::min(upper_bound, static_cast<int>(extension.available_characters.count()));
            improved |= upper_bound < previous_upper_bound;
        }
    }
    return improved;
}
