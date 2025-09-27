#include "shared_object.hpp"
#include "mdd.hpp"
#include <map>
#include <absl/container/flat_hash_set.h>

size_t calculate_flat_array_size(const mdd& data) {
    size_t size = 0;
    size += sizeof(shared_object);
    for (const auto& level : *(data.levels)) {
        size += sizeof(flat_level);
        for (const auto node : *level->nodes) {
            size += sizeof(flat_node);
            size += node->arcs_out.size() * sizeof(flat_arc);
        }
    }
    return size;
}

void serialize_initial_mdd(const mdd& mdd, shared_object* shared_object) {
    auto level_node_to_match = std::map<std::pair<int, rflcs_graph::match*>, flat_node*>();
    auto* current_pointer = reinterpret_cast<int8_t *>(shared_object);
    shared_object->num_levels = mdd.levels->size();
    current_pointer += sizeof(struct shared_object);

    for (const auto& level : *mdd.levels) {
        auto flat_level = reinterpret_cast<struct flat_level*>(current_pointer);
        flat_level->depth = level->depth;
        flat_level->num_nodes = level->nodes->size();
        current_pointer += sizeof(struct flat_level);

        for (const auto node : *level->nodes) {
            auto flat_node = reinterpret_cast<struct flat_node*>(current_pointer);
            flat_node->match_ptr = node->match;
            flat_node->character = node->match->character;
            flat_node->position_2 = node->match->extension.position_2;
            flat_node->is_active = true;
            flat_node->num_arcs_out = node->arcs_out.size();
            level_node_to_match.emplace(std::make_pair(level->depth, node->match), flat_node);
            current_pointer += sizeof(struct flat_node);
            current_pointer += sizeof(flat_arc) * node->arcs_out.size();
        }
    }

    current_pointer = reinterpret_cast<int8_t *>(shared_object);
    current_pointer += sizeof(struct shared_object);

    for (const auto& level : *mdd.levels) {
        current_pointer += sizeof(flat_level);

        for (const auto node : *level->nodes) {
            current_pointer += sizeof(flat_node);

            for (const auto arc : node->arcs_out) {
                auto flat_arc = reinterpret_cast<struct flat_arc*>(current_pointer);
                flat_arc->arc_node = level_node_to_match[std::make_pair(level->depth + 1, arc->match)];
                flat_arc->is_active = true;
                current_pointer += sizeof(struct flat_arc);
            }
        }
    }
    shared_object->active_match_count = get_active_match_count(shared_object);
}

int get_active_match_count(shared_object* flat_mdd) {
    auto static matches = absl::flat_hash_set<void*>();
    matches.clear();
    auto current_pointer =  reinterpret_cast<int8_t *>(flat_mdd);
    current_pointer += sizeof(shared_object);
    for (size_t level_index = 0; level_index < flat_mdd->num_levels; ++level_index) {
        const auto *flat_level = reinterpret_cast<struct flat_level*>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = reinterpret_cast<struct flat_node*>(current_pointer);
            if(flat_node->is_active) {
                matches.insert(flat_node->match_ptr);
            }
            current_pointer += sizeof(struct flat_node);
            current_pointer += flat_node->num_arcs_out * sizeof(struct flat_arc);
        }
    }
    return static_cast<int>(matches.size()) - 1;
}

int get_active_arc_count(shared_object* flat_mdd) {
    int active_arc_count = 0;
    auto current_pointer = reinterpret_cast<int8_t *>(&flat_mdd->flat_levels);
    for (size_t level_index = 0; level_index < flat_mdd->num_levels; ++level_index) {
        const auto *flat_level = reinterpret_cast<struct flat_level*>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            const auto *flat_node = reinterpret_cast<struct flat_node*>(current_pointer);
            current_pointer += sizeof(struct flat_node);
            for (size_t arc_index = 0; arc_index < flat_node->num_arcs_out; ++arc_index) {
                if(const auto *flat_arc = reinterpret_cast<struct flat_arc*>(current_pointer); flat_arc->is_active) {
                    active_arc_count++;
                }
                current_pointer += sizeof(flat_arc);
            }
        }
    }
    return active_arc_count;
}
