#ifndef INITIAL_MDD_HPP
#define INITIAL_MDD_HPP

#include "../../instance.hpp"

std::unique_ptr<mdd> create_initial_mdd(const instance &instance, bool forward);

void prune_by_flat_mdd(shared_object *shared_object, const mdd &mdd, mdd_node_source &mdd_node_source);

#endif
