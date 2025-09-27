
#include "sequence_enumeration_solver.h"

void set_solution(instance &instance, const std::vector<flat_node *> &stack, int pointer);

void solve_enumeration(instance &instance) {

    auto *current_pointer = reinterpret_cast<int8_t *>(&instance.shared_object->flat_levels);
    for (size_t level_index = 0; level_index < instance.shared_object->num_levels; ++level_index) {
        const auto flat_level = reinterpret_cast<const struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = reinterpret_cast<struct flat_node *>(current_pointer);
            flat_node->is_active = false;
            current_pointer += sizeof(struct flat_node);
            auto *arcs = reinterpret_cast<flat_arc *>(current_pointer);
            std::sort(arcs, arcs + flat_node->num_arcs_out, [](const flat_arc &arc_1, const flat_arc &arc_2) {
                auto match_1 = reinterpret_cast<rflcs_graph::match *>(arc_1.arc_node->match_ptr);
                auto match_2 = reinterpret_cast<rflcs_graph::match *>(arc_2.arc_node->match_ptr);
                return std::tie(match_1->extension.position_1,
                                match_1->extension.position_2)
                       <
                       std::tie(match_2->extension.position_1,
                                match_2->extension.position_2);
            });
            current_pointer += flat_node->num_arcs_out * sizeof(struct flat_arc);
        }
    }

    current_pointer = reinterpret_cast<int8_t *>(&instance.shared_object->flat_levels);
    current_pointer += sizeof(struct flat_level);
    auto *root_node = reinterpret_cast<flat_node *>(current_pointer);

    auto stack_vector = std::vector<flat_node *>();
    stack_vector.reserve(instance.alphabet_size * instance.upper_bound);

    auto stack = stack_vector.data();
    int stack_pointer = -1;
    for (size_t i = 0; i < root_node->num_arcs_out; i++) {
        stack[++stack_pointer] = root_node->arcs_out[i].arc_node;
    }

    int depth = 0;
    auto used_characters = boost::dynamic_bitset<>(instance.alphabet_size);
    used_characters.reset();
    flat_node *current_node;
    while (stack_pointer >= 0) {
        current_node = stack[stack_pointer];
        if (current_node->is_active) {
            current_node->is_active = false;
            used_characters.reset(current_node->character);
            --depth;
            --stack_pointer;
        } else {
            current_node->is_active = true;
            used_characters.set(current_node->character);
            ++depth;
            if (depth > instance.lower_bound) {
                set_solution(instance, stack_vector, stack_pointer);
                instance.lower_bound = depth;
                if(instance.lower_bound >= instance.upper_bound) {
                    instance.is_valid_solution = true;
                    instance.solver_upper_bound = instance.lower_bound;
                    return;
                }
            }
            int position_2_min = INT_MAX;
            for (size_t i = 0; i < current_node->num_arcs_out; ++i) {
                flat_node *potential_node = current_node->arcs_out[i].arc_node;
                if (position_2_min > potential_node->position_2 && !used_characters.test(potential_node->character)) {
                    stack[++stack_pointer] = potential_node;
                    position_2_min = potential_node->position_2;
                }
            }
        }
    }
    instance.solver_upper_bound = instance.lower_bound;
    instance.is_valid_solution = true;
}

void set_solution(instance &instance, const std::vector<flat_node *> &stack, const int pointer) {
    instance.solution.clear();
    for (int i = 0; i <= pointer; i++) {
        auto *node = stack[i];
        if (node->is_active) {
            instance.solution.push_back(node->character);
        }
    }
}
