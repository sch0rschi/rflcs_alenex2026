#include "header/match_metrics.hpp"

#include <algorithm>
#include <ranges>

auto get_active_match_counters(const instance& instance) -> std::vector<int>;

auto get_sums_of_combined_upper_bounds(const instance& instance) -> std::vector<int>;

auto get_having_max_combined_upper_bound_counters(const instance& instance) -> std::vector<int>;

auto aggregate_pairwise_max_in_first_vector(std::vector<int>& repetition_counter1,
                                            std::vector<int>& repetition_counter2) -> void;

auto get_characters_ordered_by_importance(const instance& instance) -> std::vector<int> {
    auto characters_with_metrics = std::vector<comparison_tuple>();
    const std::vector<int> lcs_scores = get_single_character_repetitions(instance);
    const std::vector<int> active_match_count = get_active_match_counters(instance);
    const std::vector<int> sum_of_combined_upper_bounds = get_sums_of_combined_upper_bounds(instance);
    const std::vector<int> having_max_combined_upper_bound_counters =
            get_having_max_combined_upper_bound_counters(instance);

    /*
     * Take care of selected_counter_index and the comparison operators when changing the order for sorting!
     */
    for (int character = 0; character < instance.alphabet_size; character++) {
        if (lcs_scores.at(character) > 1) {
            characters_with_metrics.emplace_back(
                character,
                lcs_scores.at(character),
                having_max_combined_upper_bound_counters.at(character),
                active_match_count.at(character),
                sum_of_combined_upper_bounds.at(character)
            );
        }
    }
    std::ranges::sort(characters_with_metrics,
                 [](const comparison_tuple& left, const comparison_tuple& right) {
                     if (std::get<1>(left) != std::get<1>(right)) {
                         return std::get<1>(left) > std::get<1>(right); // bigger is better
                     }
                     if (std::get<2>(left) != std::get<2>(right)) {
                         return std::get<2>(left) > std::get<2>(right); // bigger is better
                     }
                     if (std::get<3>(left) != std::get<3>(right)) {
                         return std::get<3>(left) > std::get<3>(right); // bigger is better
                     }
                     return std::get<4>(left) > std::get<4>(right); // bigger is better
                 });

    auto selected_rf_characters = std::vector<int>();
    for (auto tuple: characters_with_metrics) {
        selected_rf_characters.push_back(std::get<0>(tuple));
    }
    return selected_rf_characters;
}

auto get_single_character_repetitions(const instance& instance) -> std::vector<int> {
    instance.graph->matches.back().extension.repetition_counter = std::vector(instance.alphabet_size, 0);
    for (auto& [character, upper_bound, dom_succ_matches, _heur, extension]: instance.graph->matches
             | std::views::reverse | std::views::drop(1) | std::views::take(instance.graph->matches.size() - 2)) {
        if (extension.is_active) {
            extension.repetition_counter = std::vector(instance.alphabet_size, 0);

            for (auto* ancestor_match: dom_succ_matches) {
                if (ancestor_match->extension.is_active) {
                    aggregate_pairwise_max_in_first_vector(extension.repetition_counter,
                                                           ancestor_match->extension.repetition_counter);
                }
            }
            extension.repetition_counter.at(character) += 1;
        }
    }
    auto lcs_scores = std::vector(instance.alphabet_size, 0);
    for (auto* dominating_match: instance.graph->matches.front().dom_succ_matches) {
        if (dominating_match->extension.is_active) {
            aggregate_pairwise_max_in_first_vector(lcs_scores, dominating_match->extension.repetition_counter);
        }
    }
    return lcs_scores;
}

inline auto aggregate_pairwise_max_in_first_vector(std::vector<int>& repetition_counter1,
                                                   std::vector<int>& repetition_counter2) -> void {
    std::transform(repetition_counter1.begin(),
                   repetition_counter1.end(),
                   repetition_counter2.begin(),
                   repetition_counter1.begin(),
                   [](const int first_value, const int second_value) {
                       return std::max(first_value, second_value);
                   });
}

auto get_sums_of_combined_upper_bounds(const instance& instance) -> std::vector<int> {
    auto sum_of_combined_upper_bounds = std::vector(instance.alphabet_size, 0);
    for (const auto& [character, upper_bound, _dom, _heur, extension]: instance.graph->matches
             | std::views::reverse | std::views::drop(1) | std::views::take(instance.graph->matches.size() - 2)) {
        if (extension.is_active) {
            sum_of_combined_upper_bounds.at(character) += extension.combined_upper_bound;
        }
    }
    return sum_of_combined_upper_bounds;
}


auto get_active_match_counters(const instance& instance) -> std::vector<int> {
    auto counters = std::vector(instance.alphabet_size, 0);
    for (const auto& [character, upper_bound, _dom, _heur, extension]: instance.graph->matches
             | std::views::reverse | std::views::drop(1) | std::views::take(instance.graph->matches.size() - 2)) {
        if (extension.is_active) {
            counters.at(character) += 1;
        }
    }
    return counters;
}

auto get_having_max_combined_upper_bound_counters(const instance& instance) -> std::vector<int> {
    auto counters = std::vector(instance.alphabet_size, 0);
    auto max_combined_upper_bound = 0;

    for (const auto& [character, upper_bound, _dom, _heur, extension]: instance.graph->matches
             | std::views::reverse | std::views::drop(1) | std::views::take(instance.graph->matches.size() - 2)) {
        if (extension.is_active) {
            if (extension.combined_upper_bound > max_combined_upper_bound) {
                max_combined_upper_bound = extension.combined_upper_bound;
                std::ranges::fill(counters, 0);
            }
            if (extension.combined_upper_bound == max_combined_upper_bound) {
                counters.at(character) += 1;
            }
        }
    }

    return counters;
}
