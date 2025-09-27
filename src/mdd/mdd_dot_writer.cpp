#include "header/mdd_dot_writer.hpp"

#include <fstream>

#include "../formatting_utils.h"

void write_mdd_dot(const mdd& mdd, const std::string& initial_dot_filename) {
    if (std::ofstream outputFile(initial_dot_filename, std::ios::trunc); outputFile.is_open()) {
        outputFile << "digraph G {\n";

        int level_counter = 0;
        for (const auto& level: *mdd.levels) {
            outputFile << "subgraph level" << level_counter++ << " {\n";
            for (const auto node: *level->nodes) {
                outputFile << reinterpret_cast<uintptr_t>(node) << " [label=\""
                        << format_as_character(node->match->character)
                        << " [" << node->match->extension.position_1 << ","
                        << node->match->extension.position_2 << "]"
                        << "\"];\n";
            }
            outputFile << "}\n";
        }

        for (const auto& level: *mdd.levels) {
            for (const auto from: *level->nodes) {
                for (const auto to: from->arcs_out) {
                    outputFile << reinterpret_cast<uintptr_t>(from) << " -> " << reinterpret_cast<uintptr_t>(to) << "\n";
                }
            }
        }

        for (const auto& level: *mdd.levels) {
            outputFile << "{ rank=same; ";
            for (const auto node: *level->nodes) {
                outputFile << reinterpret_cast<uintptr_t>(node) << "; ";
            }
            outputFile << "}\n";
        }
        outputFile << "}" << std::endl;
        outputFile.flush();
        outputFile.close();
    } else {
        std::cerr << "Unable to open the file for writing.\n";
    }
}
