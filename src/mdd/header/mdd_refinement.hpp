#ifndef MDD_REFINEMENT_HPP
#define MDD_REFINEMENT_HPP

#include "../../instance.hpp"

void refine_mdd(const instance &instance, const mdd &mdd, character_type split_character, mdd_node_source &mdd_node_source);

#endif
