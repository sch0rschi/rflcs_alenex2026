#ifndef MDD_LEVELS_HPP
#define MDD_LEVELS_HPP

#include "mdd_node.hpp"

typedef std::vector<node *> level_nodes_type;

struct level_type {
    std::unique_ptr<level_nodes_type> nodes = std::make_unique<level_nodes_type>();
    int depth = 0;
    bool needs_pruning;

    ~level_type() {
        nodes->clear();
    }
};

typedef std::vector<std::unique_ptr<level_type>> levels_type;

#endif
