#include "header/mdd_filter.h"
#include "header/domination_utils.hpp"
#include <iomanip>
#include <ranges>
#include <absl/container/flat_hash_set.h>

#include "edge_utils.h"

bool update_nodes_and_prune(instance &instance, const mdd &mdd, mdd_node_source &mdd_node_source);

bool update_node_from_succ(const instance &instance, const level_type &level, node &node);

bool update_node_from_pred(node &node, int depth);

bool filter_succ_edges_of_node(const instance &instance, const level_type &level, node &node);

bool prune_node_from_level(const instance &instance, level_type &level, node *node);

void clear_level(level_type &level, mdd_node_source &mdd_node_source);

void filter_mdd(instance &instance, const mdd &mdd, const bool full_filter, mdd_node_source &mdd_node_source) {
    if (instance.lower_bound >= instance.upper_bound) {
        return;
    }

    if (full_filter) {
        auto is_still_running = true;
        while (is_still_running) {
            is_still_running = update_nodes_and_prune(instance, mdd, mdd_node_source);
        }
    } else {
        update_nodes_and_prune(instance, mdd, mdd_node_source);
    }
}

bool update_nodes_and_prune(instance &instance, const mdd &mdd, mdd_node_source &mdd_node_source) {
    auto is_changed = false;
    auto is_still_changing = true;

    while (is_still_changing && instance.lower_bound < instance.upper_bound) {
        is_still_changing = false;

        for (const auto &level: *mdd.levels | std::views::drop(1)) {
            for (const auto node: *level->nodes) {
                const auto needed_updates = node->needs_update_from_succ || node->needs_update_from_pred;
                const auto is_updated_from_pred = update_node_from_pred(*node, level->depth);
                is_still_changing |= is_updated_from_pred;
                if (is_updated_from_pred) {
                    is_still_changing |= filter_succ_edges_of_node(instance, *level, *node);
                }
                if (needed_updates) {
                    is_still_changing |= prune_node_from_level(instance, *level, node);
                }
            }
            clear_level(*level, mdd_node_source);
        }

        for (const auto &level: *mdd.levels | std::views::reverse | std::views::take(mdd.levels->size() - 1)) {
            for (const auto node: *level->nodes) {
                const auto needed_update_from_succ = node->needs_update_from_succ;
                const auto needed_updates = node->needs_update_from_succ || node->needs_update_from_pred;
                const auto is_updated_from_succ = update_node_from_succ(instance, *level, *node);
                is_still_changing |= is_updated_from_succ;
                if (needed_update_from_succ) {
                    is_still_changing |= filter_succ_edges_of_node(instance, *level, *node);
                }
                if (needed_updates) {
                    is_still_changing |= prune_node_from_level(instance, *level, node);
                }
            }
            clear_level(*level, mdd_node_source);
        }
        for (const auto &root_level = mdd.levels->front(); const auto node: *root_level->nodes) {
            const bool needs_update_from_succ = node->needs_update_from_succ;
            is_still_changing |= update_node_from_succ(instance, *root_level, *node);
            if (needs_update_from_succ) {
                is_still_changing |= filter_succ_edges_of_node(instance, *root_level, *node);
            }
        }

        std::erase_if(*mdd.levels, [](const std::unique_ptr<level_type> &level) { return level->nodes->empty(); });
        int levels_depth = static_cast<int>(mdd.levels->size()) - 1;
        instance.upper_bound = std::min(instance.upper_bound, levels_depth);
        instance.upper_bound = std::min(instance.upper_bound,
                                        mdd.levels->front()->nodes->front()->upper_bound_down);

        if (levels_depth > instance.upper_bound) {
            for (const auto &level: *mdd.levels | std::views::drop(instance.upper_bound + 1)) {
                for (const auto node: *level->nodes) {
                    mdd_node_source.clear_node(node);
                }
            }
            mdd.levels->resize(instance.upper_bound + 1);
        }

        if (instance.shared_object != nullptr) {
            instance.shared_object->upper_bound = instance.upper_bound;
        }
        is_changed |= is_still_changing;
    }
    return is_changed;
}

void clear_level(level_type &level, mdd_node_source &mdd_node_source) {
    if (level.needs_pruning) {
        level.needs_pruning = false;
        std::erase_if(*level.nodes, [&mdd_node_source](node *node) {
            if (node->is_active) {
                return false;
            }
            mdd_node_source.clear_node(node);
            return true;
        });
    }
}

inline bool update_node_from_succ(const instance &instance, const level_type &level, node &node) {
    if (!node.needs_update_from_succ) {
        return false;
    }
    return node.update_from_succs(level.depth, instance.lower_bound);
}

inline bool update_node_from_pred(node &node, const int depth) {
    if (!node.needs_update_from_pred || node.arcs_in.empty()) {
        return false;
    }
    return node.update_from_preds(depth);
}

inline bool prune_node_from_level(const instance &instance, level_type &level, node *node) {
    const bool no_incoming_arcs = node->arcs_in.empty();
    const bool is_insufficient_upper_bound = level.depth + node->upper_bound_down <= instance.lower_bound;
    globals::temp_character_set_1 = node->characters_on_paths_to_root;
    globals::temp_character_set_1 |= node->characters_on_paths_to_some_sink;
    globals::temp_character_set_2 = node->characters_on_all_paths_to_root;
    globals::temp_character_set_2 &= node->characters_on_all_paths_to_lower_bound_levels;
    if (no_incoming_arcs
        || is_insufficient_upper_bound
        || static_cast<int>(globals::temp_character_set_1.count()) <= instance.lower_bound
        || globals::temp_character_set_2.any()
    ) {
        level.needs_pruning = true;
        node->is_active = false;
        return true;
    }
    return false;
}

inline bool filter_succ_edges_of_node(const instance &instance, const level_type &level, node &node) {
    bool filtered_edge = false;
    int max_position_2 = std::numeric_limits<int>::max();
    static auto succ_nodes = std::vector<struct node *>();
    succ_nodes.resize(node.arcs_out.size());
    std::ranges::copy(node.arcs_out, succ_nodes.begin());
    std::ranges::sort(succ_nodes, [](const auto node1, const auto node2) {
        return node1->match->extension.position_1 < node2->match->extension.position_1;
    });
    globals::int_vector_positions_2.clear();
    for (const auto succ: succ_nodes) {
        globals::temp_character_set_1 = node.characters_on_paths_to_root;
        globals::temp_character_set_1 |= succ->characters_on_paths_to_some_sink;
        globals::temp_character_set_1.set(succ->match->character);
        const bool combined_characters_not_sufficient =
                static_cast<int>(globals::temp_character_set_1.count()) <= instance.lower_bound;
        globals::temp_character_set_2 = node.characters_on_all_paths_to_root;
        globals::temp_character_set_2 &= succ->characters_on_all_paths_to_lower_bound_levels;
        const bool repetition_free_conflict = globals::temp_character_set_2.any();
        globals::temp_character_set_1 = succ->characters_on_paths_to_some_sink;
        globals::temp_character_set_1.set(succ->match->character);
        globals::temp_character_set_1 -= node.characters_on_all_paths_to_root;
        const bool too_many_characters_already_taken =
                level.depth + static_cast<int>(globals::temp_character_set_1.count()) <= instance.lower_bound;
        const bool is_dominated = dominated_by_some_available_but_unused_character(
            succ->match->extension.position_2,
            level.depth - static_cast<int>(node.characters_on_all_paths_to_root.count()) + 1);
        if (!node.characters_on_paths_to_some_sink.test(succ->match->character)
            || level.depth + 1 + succ->upper_bound_down <= instance.lower_bound
            || combined_characters_not_sufficient
            || repetition_free_conflict
            || succ->match->extension.position_2 > max_position_2
            || is_dominated
            || too_many_characters_already_taken
            || node.characters_on_all_paths_to_root.test(succ->match->character)
        ) {
            node.unlink_pred_from_succ(succ);
            node.needs_update_from_succ = true;
            filtered_edge = true;
        } else {
            if (!node.characters_on_paths_to_root.test(succ->match->character)) {
                max_position_2 = std::min(max_position_2, succ->match->extension.position_2);
            }
            add_position_2_to_maybe_min_pos_2(succ->match->extension.position_2);
        }
    }
    return filtered_edge;
}

void filter_flat_mdd(const instance &instance, const mdd &mdd, const bool is_reporting) {
    instance.shared_object->num_levels = mdd.levels->size();
    auto static valid_matches = absl::flat_hash_set<rflcs_graph::match *>();
    valid_matches.clear();
    auto static valid_edges = absl::flat_hash_set<long>();
    valid_edges.clear();
    auto *current_pointer = reinterpret_cast<int8_t *>(instance.shared_object);
    current_pointer += sizeof(shared_object);
    for (size_t level_index = 0; level_index < instance.shared_object->num_levels; ++level_index) {
        auto static current_level_valid_matches = absl::flat_hash_set<rflcs_graph::match *>();
        current_level_valid_matches.clear();
        auto static current_level_valid_edges = absl::flat_hash_set<long>();
        current_level_valid_edges.clear();

        for (const auto node: *mdd.levels->at(level_index)->nodes) {
            current_level_valid_matches.emplace(node->match);
            valid_matches.emplace(node->match);
            for (const auto to: node->arcs_out) {
                long pair_long_encoding = match_pair_to_edge_long_encoding(node->match, to->match);
                current_level_valid_edges.emplace(pair_long_encoding);
                valid_edges.emplace(pair_long_encoding);
            }
        }

        const auto *flat_level = reinterpret_cast<struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);

        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = reinterpret_cast<struct flat_node *>(current_pointer);
            flat_node->is_active &=
                    current_level_valid_matches.contains(reinterpret_cast<rflcs_graph::match *>(flat_node->match_ptr));
            current_pointer += sizeof(struct flat_node);
            for (size_t arc_index = 0; arc_index < flat_node->num_arcs_out; ++arc_index) {
                auto flat_arc = reinterpret_cast<struct flat_arc *>(current_pointer);
                flat_arc->is_active &= current_level_valid_edges.contains(
                    match_pair_to_edge_long_encoding(
                        reinterpret_cast<const rflcs_graph::match *>(flat_node->match_ptr),
                        reinterpret_cast<const rflcs_graph::match *>(flat_arc->arc_node->match_ptr))
                );
                current_pointer += sizeof(struct flat_arc);
            }
        }
    }

    instance.shared_object->active_match_count = static_cast<int>(valid_matches.size()) - 1;
    if (is_reporting) {
        const std::chrono::duration<double> seconds_since_start = std::chrono::system_clock::now() - instance.start;
        std::cout << "Elapsed time: "
                << std::fixed << std::setprecision(2)
                << seconds_since_start.count() << "s."
                << " Reduced to " << instance.shared_object->active_match_count << " matches / "
                << 100 - 100 * static_cast<double>(instance.shared_object->active_match_count) /
                static_cast<double>(instance.graph->matches.size() - 2) << "%."
                << " Remaining edges: " << valid_edges.size() << "."
                << " Upper bound: " << instance.shared_object->upper_bound << "."
                << " Refined Characters: " << instance.shared_object->number_of_refined_characters << "."
                << std::endl;
    }
}
