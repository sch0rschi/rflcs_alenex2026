#ifndef RFLCS_MDD_HPP
#define RFLCS_MDD_HPP

#include "mdd_node_source.hpp"
#include "mdd_levels.hpp"
#include "header/character_counters_source.hpp"
#include <absl/container/flat_hash_map.h>

#include <memory>

struct mdd {
    std::unique_ptr<levels_type> levels;

    void deconstruct(mdd_node_source &mdd_node_source) const {
        for (const auto &level: *levels) {
            for (auto *node: *level->nodes) {
                mdd_node_source.clear_node(node);
            }
        }
    }

    static std::unique_ptr<mdd> copy_mdd(const mdd &original_mdd, mdd_node_source &mdd_node_source);
};

inline std::unique_ptr<mdd> mdd::copy_mdd(const mdd &original_mdd, mdd_node_source &mdd_node_source) {
    auto copy_mdd = std::make_unique<mdd>();
    copy_mdd->levels = std::make_unique<levels_type>();
    for (const auto &original_level: *original_mdd.levels) {
        auto &[copy_nodes, copy_depth, _p] = *copy_mdd->levels->emplace_back(std::make_unique<level_type>());
        copy_depth = original_level->depth;
        copy_nodes->reserve(copy_nodes->size());
        for (const auto original_node: *original_level->nodes) {
            node *copy_node = mdd_node_source.get_copy_of_old_node_with_copy_helper(original_node);
            copy_nodes->push_back(copy_node);
            for (const node *original_pred: original_node->arcs_in) {
                original_pred->copy_helper->link_pred_to_succ(copy_node);
                original_pred->copy_helper->needs_update_from_succ = false;
            }
            copy_node->needs_update_from_pred = false;
            copy_node->needs_update_from_succ = false;
        }
    }
    return copy_mdd;
}

#endif
