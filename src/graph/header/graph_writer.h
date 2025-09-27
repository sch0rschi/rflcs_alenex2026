#ifndef GRAPH_WRITER_H
#define GRAPH_WRITER_H

#include "../graph.hpp"

void write_graph_dot(const std::vector<rflcs_graph::match> &matches,
                     const std::vector<character_type> &string1,
                     const std::vector<character_type> &string2,
                     const char* initial_dot_filename,
                     bool is_reverse);

#endif
