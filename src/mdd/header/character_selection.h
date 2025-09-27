#ifndef RFLCS_CHARACTER_SELECTION_H
#define RFLCS_CHARACTER_SELECTION_H

#include <boost/timer/progress_display.hpp>

#include "../../instance.hpp"

void get_characters_ordered_by_importance_mdd(instance &instance,
                                              const mdd &reduction_mdd,
                                              mdd_node_source &mdd_node_source,
                                              const character_counters_source &character_counters_source,
                                              std::vector<character_type> &characters_ordered_by_importance, boost::timer::progress_display *progress);

#endif
