#include "globals.hpp"

int globals::alphabet_size = 0;
boost::dynamic_bitset<> globals::temp_character_set_1;
boost::dynamic_bitset<> globals::temp_character_set_2;
boost::dynamic_bitset<> globals::old_characters_on_paths_to_some_sink;
boost::dynamic_bitset<> globals::old_characters_on_all_paths_to_lower_bound_levels;
boost::dynamic_bitset<> globals::old_characters_on_paths_to_root;
boost::dynamic_bitset<> globals::old_characters_on_all_paths_to_root;
std::vector<int> globals::chaining_numbers;
std::vector<long> globals::node_character_count;
std::vector<long> globals::ingoing_arc_character_count;
std::vector<long> globals::outgoing_arc_character_count;
std::vector<int> globals::int_vector_positions_2;
