#include "graph.hpp"
#include "../instance.hpp"
#include "header/match_deactivation.hpp"
#include "match_loop_utils.h"

#include <ranges>


void release_smart_pointers(rflcs_graph::match &match);

auto deactivate_matches(instance &instance) -> bool {
    bool deactivated_a_match = false;
    instance.active_matches = 0;
    for (auto &match: instance.graph->matches
                            | std::views::drop(1)
                            | std::views::take(instance.graph->matches.size() - 2)
                            | active_match_filter) {

        match.extension.combined_upper_bound = 0;

        for (auto const &succ: match.dom_succ_matches | active_match_pointer_filter) {
            for (auto const &pred: match.extension.reversed->dom_succ_matches | active_match_pointer_filter) {
                globals::temp_character_set_1 = succ->extension.available_characters;
                globals::temp_character_set_1 |= pred->extension.available_characters;
                globals::temp_character_set_1.set(match.character);
                int specific_upper_bound = std::min(static_cast<int>(globals::temp_character_set_1.count()),
                                                    succ->upper_bound + pred->upper_bound + 1);
                match.extension.combined_upper_bound = std::max(match.extension.combined_upper_bound,
                                                                 specific_upper_bound);
            }
        }

        auto combined_upper_bound = match.extension.reversed->upper_bound + match.upper_bound - 1;
        match.extension.combined_upper_bound = std::min(match.extension.combined_upper_bound, combined_upper_bound);

        auto combined_available_characters = match.extension.available_characters;
        combined_available_characters |= match.extension.reversed->extension.available_characters;
        match.extension.combined_upper_bound = std::min(match.extension.combined_upper_bound, static_cast<int>(combined_available_characters.count()));

        match.extension.reversed->extension.combined_upper_bound = match.extension.combined_upper_bound;

        if (match.extension.combined_upper_bound <= instance.lower_bound) {
            deactivated_a_match = true;
            match.extension.is_active = false;
            match.extension.reversed->extension.is_active = false;

            release_smart_pointers(match);
            release_smart_pointers(*match.extension.reversed);
        } else {
            instance.active_matches++;
        }
    }
    return deactivated_a_match;
}

void release_smart_pointers(rflcs_graph::match &match) {
    match.extension.available_characters.resize(0);
    match.extension.repetition_counter.clear();
}
