#include "graph.hpp"
#include "header/graph_creation.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

#include "header/simple_upper_bounds.hpp"

using namespace std;
using namespace rflcs_graph;

match **create_match_matrix(const instance &instance, vector<match> &matches);

auto calculate_number_of_matches(const instance &instance) -> unsigned int;

unsigned_short_matrix create_next_occurrences(int alphabet_size, const vector<character_type> &string);

auto count_occurrences(int alphabet_size, const vector<character_type> &string) -> unique_ptr<vector<int> >;

auto manhattan_distance_comparator(const match &first, const match &second) -> bool;

void set_successor_matches(const instance &instance,
                           vector<match> &matches,
                           const unsigned_short_matrix &next_occurrences_1,
                           const unsigned_short_matrix &next_occurrences_2);

void create_matches(std::vector<match> &matches,
                    const ::instance &instance,
                    const vector<character_type> &string_1,
                    unsigned int number_of_matches,
                    const unsigned_short_matrix &next_occurrences);

void create_graph(instance &instance) {
    auto reversed_string_1 = instance.string_1;
    ranges::reverse(reversed_string_1);
    auto reversed_string_2 = instance.string_2;
    ranges::reverse(reversed_string_2);

    instance.next_occurrences_1 = create_next_occurrences(instance.alphabet_size, instance.string_1);
    instance.next_occurrences_2 = create_next_occurrences(instance.alphabet_size, instance.string_2);

    const auto reverse_next_occurrences_1 = create_next_occurrences(instance.alphabet_size, reversed_string_1);
    const auto reverse_next_occurrences_2 = create_next_occurrences(instance.alphabet_size, reversed_string_2);

    unsigned int const number_of_matches = calculate_number_of_matches(instance);

    instance.graph = make_unique<graph>();
    create_matches(instance.graph->matches, instance, instance.string_1, number_of_matches,
                   instance.next_occurrences_2);
    create_matches(instance.graph->reverse_matches, instance, reversed_string_1, number_of_matches,
                   reverse_next_occurrences_2);

    sort(instance.graph->matches.begin() + 1, instance.graph->matches.end(), manhattan_distance_comparator);
    sort(instance.graph->reverse_matches.begin() + 1, instance.graph->reverse_matches.end(),
         manhattan_distance_comparator);

    // set reverse matches
    for (unsigned long i = 0; i < instance.graph->matches.size(); i++) {
        auto &match = instance.graph->matches.at(i);
        auto &reverse_match = instance.graph->reverse_matches.at(number_of_matches - i - 1);
        match.extension.reversed = &reverse_match;
        reverse_match.extension.reversed = &match;
    }

    set_successor_matches(instance,
                          instance.graph->matches,
                          instance.next_occurrences_1,
                          instance.next_occurrences_2);
    set_successor_matches(instance,
                          instance.graph->reverse_matches,
                          reverse_next_occurrences_1,
                          reverse_next_occurrences_2);

    auto match_counter = 0;
    for (auto &
         [character, upper_bound, _dom, _heur, extension]: instance.graph->matches) {
        extension.match_id = match_counter++;
    }
    for (auto &[character, upper_bound, _dom, _heur, extension]: instance.graph->reverse_matches) {
        extension.match_id = match_counter++;
    }

    delete reverse_next_occurrences_1;
    delete reverse_next_occurrences_2;

    setup_matches_for_simple_upper_bound(instance);
}

auto calculate_number_of_matches(const instance &instance) -> unsigned int {
    const auto occurrences_1 = count_occurrences(instance.alphabet_size, instance.string_1);
    const auto occurrences_2 = count_occurrences(instance.alphabet_size, instance.string_2);
    unsigned int number_of_matches = 2;
    for (int character = 0; character < instance.alphabet_size; character++) {
        number_of_matches += occurrences_1->at(character) * occurrences_2->at(character);
    }
    return number_of_matches;
}

void create_matches(std::vector<match> &matches,
                    const ::instance &instance,
                    const vector<character_type> &string_1,
                    const unsigned int number_of_matches,
                    const unsigned_short_matrix &next_occurrences) {
    matches.resize(number_of_matches);
    for (auto &[character, upper_bound, _dom, _heur, extension]: matches) {
        extension = match_extension();
    }
    auto &[root_character, root_upper_bound, _dom_root, _heur_root, root_extension] = matches.front();
    root_extension.position_1 = 0;
    root_character = SHRT_MAX;
    root_extension.position_2 = 0;
    auto &[leaf_character, leaf_upper_bound, _dom_leaf, _heur_leaf, leaf_extension] = matches.back();
    leaf_extension.position_1 = static_cast<int>(instance.string_1.size());
    leaf_character = SHRT_MAX;
    leaf_extension.position_2 = static_cast<int>(instance.string_2.size());

    unsigned int match_counter = 1;
    for (int position_1 = 0; position_1 < static_cast<int>(instance.string_1.size()); position_1++) {
        const auto string_character = string_1.at(position_1);
        auto const start_position_2 = next_occurrences[string_character];
        for (short position_2 = start_position_2; position_2 < static_cast<int>(instance.string_2.size()); match_counter++) {
            auto &[character, upper_bound, _dom, _heur, extension] = matches.at(match_counter);
            character = string_character;
            extension.position_1 = position_1;
            extension.position_2 = position_2;
            if (position_2 < static_cast<int>(instance.string_2.size()) - 1) {
                position_2 = next_occurrences[(position_2 + 1) * instance.alphabet_size + string_character];
            } else {
                position_2 = static_cast<int>(instance.string_2.size());
            }
        }
    }
}

match **create_match_matrix(const instance &instance, vector<match> &matches) {
    auto match_matrix = new match *[instance.string_1.size() * instance.string_2.size()];
    for (auto &match: matches | std::views::drop(1) | std::views::take(matches.size() - 2)) {
        match_matrix[match.extension.position_1 * instance.string_2.size() + match.extension.position_2] = &match;
    }
    return match_matrix;
}


unsigned_short_matrix create_next_occurrences(const int alphabet_size, const vector<character_type> &string) {
    auto character_next_occurrences = new short[string.size() * alphabet_size];
    for (int i = 0; i < static_cast<int>(string.size()) * alphabet_size; i++) {
        character_next_occurrences[i] = string.size();
    }

    for (int position = 0; position < static_cast<int>(string.size()); position++) {
        character_next_occurrences[position * alphabet_size + string.at(position)] = position;
    }

    auto last_positions = vector<short>(alphabet_size);
    for (int character = 0; character < alphabet_size; character++) {
        last_positions.at(character) = character_next_occurrences[alphabet_size * (string.size() - 1) + character];
    }

    for (auto position = static_cast<short>(string.size() - 2); position >= 0; --position) {
        for (auto character = 0; character < alphabet_size; character++) {
            character_next_occurrences[position * alphabet_size + character] = std::min(character_next_occurrences[position * alphabet_size + character], last_positions.at(character));
            last_positions.at(character) = character_next_occurrences[position * alphabet_size + character];
        }
    }
    return character_next_occurrences;
}

auto count_occurrences(int alphabet_size,
                       const vector<character_type> &string) -> unique_ptr<vector<int> > {
    auto occurrences_counter = make_unique<vector<int> >(alphabet_size);
    for (auto &count: *occurrences_counter) {
        count = 0;
    }
    for (const int position: string) {
        occurrences_counter->at(position) += 1;
    }
    return occurrences_counter;
}

void set_successor_matches(const instance &instance,
                           vector<match> &matches,
                           const unsigned_short_matrix &next_occurrences_1,
                           const unsigned_short_matrix &next_occurrences_2) {
    const auto match_matrix = create_match_matrix(instance, matches);

    const auto string_1_length = instance.string_1.size();
    const auto string_2_length = instance.string_2.size();
    for (auto &match: matches | std::views::take(matches.size() - 1)) {
        match.extension.succ_matches.reserve(instance.alphabet_size);
        for (int character = 0; character < instance.alphabet_size; character++) {
            if (character != match.character) {
                const auto next_position_1 = next_occurrences_1[match.extension.position_1 * instance.alphabet_size + character];
                const auto next_position_2 = next_occurrences_2[match.extension.position_2 * instance.alphabet_size + character];
                if (next_position_1 < static_cast<int>(string_1_length) &&
                    next_position_2 < static_cast<int>(string_2_length)) {
                    auto succ_match = match_matrix[next_position_1 * string_2_length + next_position_2];
                    match.extension.succ_matches.push_back(succ_match);
                }
            }
        }
        match.extension.succ_matches.shrink_to_fit();

        static auto succ_matches_copy = vector<struct match *>(string_1_length);
        std::ranges::fill(succ_matches_copy, nullptr);
        for (const auto succ_match: match.extension.succ_matches) {
            succ_matches_copy[succ_match->extension.position_1] = succ_match;
        }

        auto succ_match_counter = 0;
        for (const auto succ_match: succ_matches_copy | std::views::filter([](auto *succ_match_filter) {
            return succ_match_filter != nullptr;
        })) {
            match.extension.succ_matches.at(succ_match_counter++) = succ_match;
        }

        match.dom_succ_matches.reserve(succ_match_counter);
        int smallest_position_2 = static_cast<int>(string_2_length) + 1;
        std::ranges::copy_if(match.extension.succ_matches, std::back_inserter(match.dom_succ_matches),
                             [&smallest_position_2](const struct match *dominating_match_candidate) {
                                 const bool is_dominating =
                                         dominating_match_candidate->extension.position_2 < smallest_position_2;
                                 smallest_position_2 = std::min(smallest_position_2,
                                                                dominating_match_candidate->extension.position_2);
                                 return is_dominating;
                             });
        match.dom_succ_matches.shrink_to_fit();
        if (match.dom_succ_matches.empty()) {
            match.dom_succ_matches.push_back(&matches.back());
        }
    }

    delete match_matrix;
}

auto manhattan_distance_comparator(const match &first, const match &second) -> bool {
    const auto first_distance = first.extension.position_1 + first.extension.position_2;
    const auto second_distance = second.extension.position_1 + second.extension.position_2;
    if (first_distance != second_distance) {
        return first_distance < second_distance;
    }
    return first.extension.position_1 < second.extension.position_1;
}
