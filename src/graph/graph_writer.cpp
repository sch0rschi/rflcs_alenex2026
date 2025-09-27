#include "header/graph_writer.h"

#include <fstream>
#include <ranges>

#include "../formatting_utils.h"
#include "graph.hpp"
#include "match_loop_utils.h"

void write_graph_dot(const std::vector<rflcs_graph::match> &matches,
                     const std::vector<character_type> &string1,
                     const std::vector<character_type> &string2,
                     const char *initial_dot_filename,
                     const bool is_reverse) {
    if (std::ofstream outputFile(initial_dot_filename, std::ios::trunc); outputFile.is_open()) {
        outputFile << "digraph G {\n";

        for (const auto &[character, _u, _dom, _heur, extension]: matches
                                                                  | std::views::drop(1)
                                                                  | std::views::take(matches.size() - 2)
                                                                  | active_match_filter) {

            int position_1 = is_reverse ? extension.reversed->extension.position_1 : extension.position_1;
            int position_2 = is_reverse ? extension.reversed->extension.position_2 : extension.position_2;

            outputFile << extension.match_id << " [label=\""
                    << format_as_character(character)
                    << "\" pos=\"" << static_cast<double>(position_1) / 4
                    << "," << static_cast<double>(position_2) / 4 << "!"
                    << "\"];\n";
        }

        for (const auto &[_c, _u, dom_succ, _heur, extension]: matches
                                                               | std::views::drop(1)
                                                               | std::views::take(matches.size() - 2)
                                                               | active_match_filter) {
            for (const auto &to: dom_succ) {
                if (to != &matches.front() && to != &matches.back()) {
                        outputFile << extension.match_id << " -> " << to->extension.match_id << "\n";
                }
            }
        }

        int counter = 0;
        for (const auto character: string1) {
            outputFile << "X" << counter << " [label=\"" << format_as_character(character) << "\", "
                    << "pos=\"" << static_cast<double>(counter) / 4 << ", -0.5!\", shape=none];\n";
            counter++;
        }

        counter = 0;
        for (const auto character: string2) {
            outputFile << "Y" << counter << " [label=\"" << format_as_character(character) << "\", "
                    << "pos=\"-0.5," << static_cast<double>(counter) / 4 << "!\", shape=none];\n";
            counter++;
        }

        outputFile << "}" << std::endl;
        outputFile.flush();
        outputFile.close();
    } else {
        std::cerr << "Unable to open the file for writing.\n";
    }
}
