#ifndef RFLCS_MATCH_LOOP_UTILS_H
#define RFLCS_MATCH_LOOP_UTILS_H

#include <ranges>

#include "graph.hpp"

static constexpr auto active_match_pointer_filter = std::ranges::views::filter([](const rflcs_graph::match *maybe) { return maybe->extension.is_active; });
static constexpr auto active_match_filter = std::ranges::views::filter([](rflcs_graph::match const &maybe) { return maybe.extension.is_active; });

#endif
