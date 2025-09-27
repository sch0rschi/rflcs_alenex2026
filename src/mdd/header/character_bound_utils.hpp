#ifndef CHARACTER_BOUND_UTILS_HPP
#define CHARACTER_BOUND_UTILS_HPP

#include <boost/dynamic_bitset.hpp>

bool are_enough_characters_available(int lower_bound,
                                     int depth,
                                     const boost::dynamic_bitset<> &pred_available_characters,
                                     const boost::dynamic_bitset<> &succ_available_characters);

#endif