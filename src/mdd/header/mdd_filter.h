#ifndef RFLCS_MDD_FILTER_H
#define RFLCS_MDD_FILTER_H

#include "../../instance.hpp"

void filter_mdd(instance &instance, const mdd &mdd, bool full_filter, mdd_node_source &mdd_node_source);

void filter_flat_mdd(const instance &instance, const mdd &mdd, bool is_reporting);

#endif
