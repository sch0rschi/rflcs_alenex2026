#ifndef UTILS_H
#define UTILS_H
#include "instance.hpp"
#include "graph/graph.hpp"

std::vector<rflcs_graph::match *> get_active_matches_vector(const instance &instance);

long sanitize_memory_usage(long ru_maxrss);

#endif
