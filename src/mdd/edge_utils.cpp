#include "edge_utils.h"

#include "../graph/graph.hpp"

long match_pair_to_edge_long_encoding(const rflcs_graph::match *from, const rflcs_graph::match *to) {
    return (static_cast<long>(from->extension.match_id) << 32) + static_cast<long>(to->extension.match_id);
}
