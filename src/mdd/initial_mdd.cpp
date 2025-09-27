#include "header/initial_mdd.hpp"
#include "header/domination_utils.hpp"
#include "header/character_bound_utils.hpp"
#include "mdd.hpp"
#include "../instance.hpp"
#include "../graph/graph.hpp"

#include <memory>
#include <vector>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

std::unique_ptr<mdd> create_initial_mdd(const instance &instance, bool forward) {

    std::unique_ptr<mdd> mdd = std::make_unique<struct mdd>();
    mdd->levels = std::make_unique<levels_type>();

    instance.mdd_node_source->sort_cache();

    auto &[root_level, first_level_depth, _rp] = *mdd->levels->emplace_back(std::make_unique<level_type>());
    const auto root_node = instance.mdd_node_source->new_node();
    root_node->is_active = true;
    auto &root_match = forward ? instance.graph->matches.front() : instance.graph->reverse_matches.front();
    root_node->match = &root_match;
    root_node->characters_on_paths_to_root = boost::dynamic_bitset<>(globals::alphabet_size);
    root_node->characters_on_all_paths_to_root = boost::dynamic_bitset<>(globals::alphabet_size);
    root_node->characters_on_paths_to_some_sink = root_match.extension.available_characters;
    root_node->characters_on_all_paths_to_lower_bound_levels = boost::dynamic_bitset<>(globals::alphabet_size);
    root_node->upper_bound_down = instance.upper_bound;

    root_level->push_back(root_node);

    auto match_to_node_map = absl::flat_hash_map<rflcs_graph::match *, node *>();
    while (!mdd->levels->back()->nodes->empty() &&
           mdd->levels->back()->depth < instance.upper_bound) {
        auto &[current_nodes, current_depth, _p] = *mdd->levels->back();
        auto &[next_nodes, next_depth, _np] = *mdd->levels->emplace_back(std::make_unique<level_type>());
        next_depth = current_depth + 1;
        match_to_node_map.clear();
        for (const auto pred_node: *current_nodes) {
            int min_position_2 = std::numeric_limits<int>::max();
            globals::int_vector_positions_2.clear();
            for (auto succ_match: pred_node->match->extension.succ_matches) {
                const bool not_dominated = !dominated_by_some_available_but_unused_character(
                        succ_match->extension.position_2, current_depth);
                if (succ_match->character < instance.alphabet_size
                    && next_depth <= succ_match->extension.reversed->upper_bound
                    && succ_match->extension.position_2 < min_position_2
                    && pred_node->characters_on_paths_to_some_sink.test(succ_match->character)
                    && next_depth + succ_match->upper_bound > instance.lower_bound
                    && not_dominated
                    && are_enough_characters_available(instance.lower_bound,
                                                       next_depth,
                                                       pred_node->match->extension.reversed->extension.available_characters,
                                                       succ_match->extension.available_characters)
                        ) {
                    if (!pred_node->match->extension.reversed->extension.available_characters.test(
                            succ_match->character)) {
                        min_position_2 = std::min(min_position_2, succ_match->extension.position_2);
                    } else {
                        add_position_2_to_maybe_min_pos_2(succ_match->extension.position_2);
                    }
                    if (succ_match->extension.is_active) {
                        node *succ_node;
                        if (match_to_node_map.contains(succ_match)) {
                            succ_node = match_to_node_map.at(succ_match);
                        } else {
                            succ_node = instance.mdd_node_source->get_new_node_with_match(*succ_match);
                            match_to_node_map.insert({succ_match, succ_node});
                            next_nodes->push_back(succ_node);
                        }
                        pred_node->link_pred_to_succ(succ_node);
                    }
                }
            }
        }
    }
    while (mdd->levels->back()->nodes->empty()) {
        mdd->levels->pop_back();
    }

    return mdd;
}

void prune_by_flat_mdd(shared_object *shared_object, const mdd &mdd, mdd_node_source &mdd_node_source) {
    auto *current_pointer = reinterpret_cast<int8_t *>(&shared_object->flat_levels);

    auto static valid_matches_sets = std::vector<absl::flat_hash_set<rflcs_graph::match *> >(mdd.levels->size());
    valid_matches_sets.resize(mdd.levels->size());
    for (auto &valid_matches: valid_matches_sets) {
        valid_matches.clear();
    }

    static auto valid_edges_sets =
        std::vector<absl::flat_hash_set<std::pair<rflcs_graph::match *, rflcs_graph::match*> > >(mdd.levels->size());
    valid_edges_sets.resize(mdd.levels->size());
    for (auto &valid_edges: valid_edges_sets) {
        valid_edges.clear();
    }

    for (size_t level_index = 0; level_index < shared_object->num_levels; ++level_index) {
        auto &current_level_valid_matches = valid_matches_sets.at(level_index);
        auto &current_level_valid_edges = valid_edges_sets.at(level_index);
        const auto *flat_level = reinterpret_cast<struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);

        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = reinterpret_cast<struct flat_node *>(current_pointer);
            auto *match = reinterpret_cast<rflcs_graph::match *>(flat_node->match_ptr);
            if(flat_node->is_active) {
                current_level_valid_matches.insert(match);
            }
            current_pointer += sizeof(struct flat_node);

            for (size_t arc_index = 0; arc_index < flat_node->num_arcs_out; ++arc_index) {
                if(auto flat_arc = reinterpret_cast<struct flat_arc *>(current_pointer); flat_arc->is_active) {
                    auto *to = reinterpret_cast<rflcs_graph::match *>(flat_arc->arc_node->match_ptr);
                    current_level_valid_edges.emplace(match, to);
                }
                current_pointer += sizeof(flat_arc);
            }
        }
    }

    for (const auto &level: *mdd.levels) {
        const auto &current_valid_matches = valid_matches_sets.at(level->depth);
        const auto &current_valid_edges = valid_edges_sets.at(level->depth);
        for (std::vector nodes(level->nodes->begin(), level->nodes->end()); const auto node: nodes) {
            if (!current_valid_matches.contains(node->match)) {
                std::erase(*level->nodes, node);
                mdd_node_source.clear_node(node);
            } else {
                for (std::vector succs(node->arcs_out.begin(), node->arcs_out.end()); const auto succ: succs) {
                    if (!current_valid_edges.contains({node->match, succ->match})) {
                        node->unlink_pred_from_succ(succ);
                    }
                }
            }
        }
    }
}
