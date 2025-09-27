#ifndef GLOBALS_HPP
#define GLOBALS_HPP
#include <boost/dynamic_bitset.hpp>
#include <vector>

struct globals {
    static int alphabet_size;
    static boost::dynamic_bitset<> temp_character_set_1;
    static boost::dynamic_bitset<> temp_character_set_2;
    static boost::dynamic_bitset<> old_characters_on_paths_to_some_sink;
    static boost::dynamic_bitset<> old_characters_on_all_paths_to_lower_bound_levels;
    static boost::dynamic_bitset<> old_characters_on_paths_to_root;
    static boost::dynamic_bitset<> old_characters_on_all_paths_to_root;
    static std::vector<int> chaining_numbers;
    static std::vector<long> node_character_count;
    static std::vector<long> outgoing_arc_character_count;
    static std::vector<long> ingoing_arc_character_count;
    static std::vector<int> int_vector_positions_2;
};

#endif
