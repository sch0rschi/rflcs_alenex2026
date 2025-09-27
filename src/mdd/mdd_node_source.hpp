#ifndef MDD_NODE_SOURCE_HPP
#define MDD_NODE_SOURCE_HPP

#include "mdd_node.hpp"
#include <algorithm>

struct mdd_node_source {

private:
    std::vector<node *> cache= std::vector<node *>();

public:
    void clear_node(node *node) {
        node->clear();
        cache.push_back(node);
    }

    void sort_cache() {
        std::ranges::sort(cache);
    }

    [[nodiscard]] node* new_node() {
        if(!cache.empty()) {
            const auto node = cache.back();
            cache.pop_back();
            return node;
        }
        return new node();
    }

    [[nodiscard]] node *get_new_node_with_match(rflcs_graph::match &match) {

        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
            fresh_node->characters_on_all_paths_to_root = boost::dynamic_bitset<>(globals::alphabet_size);
            fresh_node->characters_on_paths_to_some_sink = match.extension.available_characters;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = boost::dynamic_bitset<>(globals::alphabet_size);
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
            fresh_node->characters_on_all_paths_to_root.reset();
            fresh_node->characters_on_paths_to_some_sink = match.extension.available_characters;
            fresh_node->characters_on_all_paths_to_lower_bound_levels.reset();
        }
        fresh_node->is_active = true;
        fresh_node->match = &match;

        fresh_node->characters_on_paths_to_root.set(fresh_node->match->character);
        fresh_node->characters_on_all_paths_to_root.set(fresh_node->match->character);
        fresh_node->characters_on_paths_to_some_sink.reset(match.character);

        fresh_node->upper_bound_down = match.upper_bound;
        fresh_node->needs_update_from_pred = true;
        fresh_node->needs_update_from_succ = true;
        return fresh_node;
    }

    [[nodiscard]] node *get_copy_of_old_node(const node &old_node) {
        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = old_node.characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node.characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node.characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node.characters_on_all_paths_to_lower_bound_levels;
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = old_node.characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node.characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node.characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node.characters_on_all_paths_to_lower_bound_levels;
        }
        fresh_node->is_active = true;
        fresh_node->match = old_node.match;
        fresh_node->upper_bound_down = old_node.upper_bound_down;

        fresh_node->needs_update_from_pred = true;
        fresh_node->needs_update_from_succ = true;
        return fresh_node;
    }

    [[nodiscard]] node *get_copy_of_old_node_with_copy_helper(node *old_node) {
        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = old_node->characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node->characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node->characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node->characters_on_all_paths_to_lower_bound_levels;
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = old_node->characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node->characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node->characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node->characters_on_all_paths_to_lower_bound_levels;
        }
        fresh_node->is_active = true;
        fresh_node->match = old_node->match;
        fresh_node->upper_bound_down = old_node->upper_bound_down;

        fresh_node->needs_update_from_pred = false;
        fresh_node->needs_update_from_succ = false;

        fresh_node->copy_helper = old_node;
        old_node->copy_helper = fresh_node;

        return fresh_node;
    }

    ~mdd_node_source() {
        cache.clear();
    }
};

#endif
