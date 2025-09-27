#include "utils.h"

#include <ranges>

std::vector<rflcs_graph::match *> get_active_matches_vector(const instance &instance) {
    auto matches = std::vector<rflcs_graph::match *>();
    for (auto &match: instance.graph->matches | std::ranges::views::drop(1) |
                      std::ranges::views::take(instance.graph->matches.size() - 2)) {
        if (match.extension.is_active) {
            matches.push_back(&match);
        }
                      }
    return matches;
}

long sanitize_memory_usage(long ru_maxrss) {
#if defined(__APPLE__)
    return ru_maxrss / 1024;
#else
    return ru_maxrss;
#endif
}
