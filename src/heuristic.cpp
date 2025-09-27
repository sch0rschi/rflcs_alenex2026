#include "config.hpp"
#include "heuristic.hpp"
#include "instance.hpp"
#include "reduction_orchestration.hpp"
#include "globals.hpp"

#include <boost/timer/progress_display.hpp>

#include "graph/graph.hpp"

#include <cmath>
#include <algorithm>
#include <random>
#include <ranges>
#include <set>
#include <vector>

auto combine(instance &instance, std::vector<rflcs_graph::match> &matches, bool is_building_from_back) -> void;

auto setup(const instance &instance) -> void;

auto clear(const instance &instance) -> void;

auto
set_heuristic_solution(instance &instance, const rflcs_graph::match &match, bool is_building_from_back) -> void;

auto heuristic_solve(instance &instance) -> void {
    setup(instance);

    int lower_bound_before_round = 0;
    instance.lower_bound = 0;

    int number_of_bad_runs = 0;
    int const number_of_bad_runs_limit = static_cast<int>(instance.string_1.size() * instance.string_2.size())
            / instance.alphabet_size;
    int reset_counter = 0;
    boost::timer::progress_display progress_display(number_of_bad_runs_limit);

    const auto reset_limit = round(sqrt(number_of_bad_runs_limit));
    while (instance.lower_bound < instance.upper_bound &&
           (lower_bound_before_round < instance.lower_bound || number_of_bad_runs < number_of_bad_runs_limit)) {
        lower_bound_before_round = instance.lower_bound;

        combine(instance, instance.graph->matches, true);
        combine(instance, instance.graph->reverse_matches, false);

        if (lower_bound_before_round < instance.lower_bound) {
            reduce_graph_while_heuristic(instance);
            number_of_bad_runs = 0;
            progress_display.restart(number_of_bad_runs_limit);
            reset_counter = 0;
        } else {
            number_of_bad_runs++;
            ++progress_display;
            reset_counter++;
        }
        if (reset_counter > reset_limit) {
            reset_counter = 0;
            clear(instance);
        }
    }
}

void setup(const instance &instance) {
    for (auto &[character, upper_bound, _dom, heuristic_characters, extension]: instance.graph->matches) {
        heuristic_characters = boost::dynamic_bitset<>(instance.alphabet_size);
        extension.heuristic_previous_match = nullptr;
        extension.reversed->heuristic_characters = boost::dynamic_bitset<>(instance.alphabet_size);
        extension.reversed->extension.heuristic_previous_match = nullptr;
        if (character < instance.alphabet_size) {
            heuristic_characters.set(character);
            extension.reversed->heuristic_characters.set(character);
        }
    }
}

void clear(const instance &instance) {
    for (auto &[character, upper_bound, _dom, heuristic_characters, extension]: instance.graph->matches) {
        if (extension.is_active) {
            heuristic_characters.reset();
            extension.reversed->heuristic_characters.reset();
            if (character < instance.alphabet_size) {
                heuristic_characters.set(character);
                extension.reversed->heuristic_characters.set(character);
            }
        }
    }
}

void combine(instance &instance, std::vector<rflcs_graph::match> &matches, const bool is_building_from_back) {
    auto static candidate_matches = std::vector<rflcs_graph::match *>(instance.alphabet_size);
    for (auto &current_match: std::ranges::reverse_view(matches)) {
        if (current_match.extension.is_active
            && !current_match.dom_succ_matches.empty()) {
            int position = 0;
            unsigned long best_heuristic_score = 0;
            for (auto *potential_match: current_match.dom_succ_matches) {
                if (potential_match->extension.is_active) {
                    globals::temp_character_set_1 = current_match.extension.reversed->heuristic_characters;
                    globals::temp_character_set_1 |= potential_match->heuristic_characters;
                    if (unsigned long const heuristic_score = globals::temp_character_set_1.count();
                            heuristic_score > best_heuristic_score) {
                        best_heuristic_score = heuristic_score;
                        position = 0;
                        candidate_matches[position++] = potential_match;
                    } else if (heuristic_score == best_heuristic_score) {
                        candidate_matches[position++] = potential_match;
                    }
                }
            }

            std::uniform_int_distribution uniform_distribution(0, position - 1);

            auto &chosen_match = *candidate_matches.at(uniform_distribution(instance.random));
            current_match.extension.heuristic_previous_match = &chosen_match;
            current_match.heuristic_characters = chosen_match.heuristic_characters;
            if (current_match.character < instance.alphabet_size) {
                current_match.heuristic_characters.set(current_match.character);
            }

            if (instance.lower_bound < static_cast<int>(best_heuristic_score) - HEURISTIC_SOLUTION_DECREMENTER) {
                instance.lower_bound = static_cast<int>(best_heuristic_score) - HEURISTIC_SOLUTION_DECREMENTER;
                set_heuristic_solution(instance, current_match, is_building_from_back);
            }
        }
    }
}

void
set_heuristic_solution(::instance &instance, const rflcs_graph::match &match, const bool is_building_from_back) {
    instance.heuristic_solution_time = std::chrono::system_clock::now();
    instance.solution.clear();
    auto characters = std::set<int>();

    const auto *actual_match = &match;
    while (actual_match != nullptr && actual_match->character < instance.alphabet_size) {
        if (!characters.contains(actual_match->character)) {
            instance.solution.push_front(actual_match->character);
            characters.insert(actual_match->character);
        }
        actual_match = actual_match->extension.heuristic_previous_match;
    }

    actual_match = match.extension.reversed;
    while (actual_match != nullptr && actual_match->character < instance.alphabet_size) {
        if (!characters.contains(actual_match->character)) {
            instance.solution.push_back(actual_match->character);
            characters.insert(actual_match->character);
        }
        actual_match = actual_match->extension.heuristic_previous_match;
    }

    if (is_building_from_back) {
        std::ranges::reverse(instance.solution);
    }
}
