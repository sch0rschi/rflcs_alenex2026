#include "header/mdd_refinement.hpp"

#include <ranges>
#include <chrono>

void refine_mdd_level(const level_type &level,
                      const instance &instance,
                      character_type split_character,
                      mdd_node_source &mdd_node_source);

void split_node(node *node_yes_no,
                character_type split_character,
                const level_type &level,
                const instance &instance,
                mdd_node_source &mdd_node_source);

void refine_mdd(const instance &instance, const mdd &mdd, const character_type split_character,
                mdd_node_source &mdd_node_source) {
    for (const auto &level: *mdd.levels | std::views::drop(1)) {
        refine_mdd_level(*level, instance, split_character, mdd_node_source);
    }
}

void refine_mdd_level(const level_type &level,
                      const instance &instance,
                      const character_type split_character,
                      mdd_node_source &mdd_node_source) {
    static auto nodes = std::vector<node *>();
    nodes.resize(level.nodes->size());
    std::ranges::copy(*level.nodes, nodes.begin());
    for (const auto node: nodes) {
        if (node->characters_on_paths_to_root.test(split_character)
            && node->characters_on_paths_to_some_sink.test(split_character)) {
            split_node(node, split_character, level, instance, mdd_node_source);
        }
    }
}

inline void split_node(node *node_yes_no,
           const character_type split_character,
           const level_type &level,
           const instance &instance,
           mdd_node_source &mdd_node_source) {
    node *node_no_maybe = mdd_node_source.get_copy_of_old_node(*node_yes_no);
    node_no_maybe->characters_on_paths_to_root.reset(split_character);
    node_no_maybe->characters_on_all_paths_to_root.reset(split_character);

    node_yes_no->characters_on_paths_to_some_sink.reset(split_character);
    node_yes_no->characters_on_all_paths_to_root.set(split_character);

    node_yes_no->notify_relatives_of_update();
    node_yes_no->needs_update_from_succ = true;
    node_yes_no->needs_update_from_pred = true;
    node_no_maybe->needs_update_from_succ = true;
    node_no_maybe->needs_update_from_pred = true;

    auto static arcs_in_copy = std::vector<node *>();
    arcs_in_copy.resize(node_yes_no->arcs_in.size());
    std::ranges::copy(node_yes_no->arcs_in, arcs_in_copy.begin());
    for (const auto pred: arcs_in_copy) {
        if (!pred->characters_on_all_paths_to_root.test(split_character)) {
            pred->link_pred_to_succ(node_no_maybe);
            pred->unlink_pred_from_succ(node_yes_no);
        }
    }

    auto static arcs_out_copy = std::vector<node *>();
    arcs_out_copy.resize(node_yes_no->arcs_out.size());
    std::ranges::copy(node_yes_no->arcs_out, arcs_out_copy.begin());
    for (const auto succ: arcs_out_copy) {
        node_no_maybe->link_pred_to_succ(succ);
        if (succ->characters_on_all_paths_to_lower_bound_levels.test(split_character) || succ->match->character == split_character) {
            node_yes_no->unlink_pred_from_succ(succ);
        }
    }

    if (node_no_maybe->arcs_in.empty() ||
        (node_no_maybe->arcs_out.empty() && level.depth <= instance.lower_bound)) {
        mdd_node_source.clear_node(node_no_maybe);
    } else {
        level.nodes->push_back(node_no_maybe);
    }

    if (node_yes_no->arcs_in.empty()
        || (node_yes_no->arcs_out.empty() && level.depth <= instance.lower_bound)
        || static_cast<int>(node_yes_no->characters_on_all_paths_to_root.count()) > level.depth) {
        node_yes_no->deactivate();
    }
}
