#include <ranges>
#include <iomanip>
#include <sys/mman.h>
#ifdef __linux__
#include <sys/prctl.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif
#include <sys/wait.h>
#include "reduction_orchestration.hpp"

#include <fstream>

#include "instance.hpp"
#include "graph/header/rf_subset_lcs_relaxation.hpp"

#include "graph/header/simple_upper_bounds.hpp"
#include "graph/header/match_deactivation.hpp"
#include "graph/header/reduce_graph.hpp"
#include "mdd/header/initial_mdd.hpp"
#include "mdd/shared_object.hpp"
#include "mdd/header/mdd_filter.h"
#include "mdd/header/mdd_dot_writer.hpp"
#include "config.hpp"

#include <iostream>
#include <unistd.h>
#include <csignal>
#include <absl/container/flat_hash_set.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/resource.h>
#include <cstdlib>

#include "utils.h"
#include "mdd/header/mdd_reduction.hpp"

void handle_threads_for_mdd_reduction(instance &instance);

void set_oom_score_adj(int score);

void filter_matches_by_flat_mdd(instance &instance);

void reduce_graph_pre_solver_by_mdd(instance &instance);

void timeout_handler(int signal);

double get_elapsed_seconds(const instance &instance);

long calculate_mdd_complexity(const mdd &mdd);

void reduce_graph_while_heuristic(instance &instance) {
    bool is_improving = true;
    while (is_improving) {
        if (instance.lower_bound >= instance.upper_bound) {
            return;
        }
        is_improving = false;
        is_improving |= calculate_simple_upper_bounds(instance);
        is_improving |= deactivate_matches(instance);
        reduce_graph(instance);
    }
    const std::chrono::duration<double> heuristic_elapsed_seconds = std::chrono::system_clock::now() - instance.start;
    std::cout << std::fixed << std::setprecision(2)
            << "\n"
            << "Heuristic found solution with length " << instance.lower_bound << ", took "
            << heuristic_elapsed_seconds.count() << "s. Reducing to " << instance.active_matches << " nodes = "
            << 100.0 * (1
                        - static_cast<double>(instance.active_matches)
                        / static_cast<double>(instance.graph->matches.size() - 2)) << "%."
            << std::endl;
}

void reduce_graph_pre_solver(instance &instance) {
    bool is_improving = true;
    while (is_improving) {
        if (instance.lower_bound >= instance.upper_bound) {
            return;
        }
        is_improving = false;
        is_improving |= relax_by_fixed_character_rf_constraint(instance);
        is_improving |= calculate_simple_upper_bounds(instance);
        is_improving |= deactivate_matches(instance);
        reduce_graph(instance);
        std::cout << "Repetition-Free Subset LCS Relaxation reduced to " << instance.active_matches << " matches = "
                    << 100.0 * (1 - static_cast<double>(instance.active_matches)
                                / static_cast<double>(instance.graph->matches.size() - 2)) << "%."
                    << std::endl;
    }

    return reduce_graph_pre_solver_by_mdd(instance);
}

void reduce_graph_pre_solver_by_mdd(instance &instance) {
    auto forward_mdd = create_initial_mdd(instance, true);
    filter_mdd(instance, *forward_mdd, true, *instance.mdd_node_source);
    const long forward_mdd_complexity = calculate_mdd_complexity(*forward_mdd);
    auto backward_mdd = create_initial_mdd(instance, false);
    filter_mdd(instance, *backward_mdd, true, *instance.mdd_node_source);
    const long backward_mdd_complexity = calculate_mdd_complexity(*backward_mdd);

    instance.is_solving_forward = forward_mdd_complexity < backward_mdd_complexity;
    std::cout << "Mdd forward complexity: " << forward_mdd_complexity
            << ", mdd backward complexity: " << backward_mdd_complexity
            << ". Reduction is using forward problem: "
            << std::boolalpha << instance.is_solving_forward << "." << std::endl;
    instance.mdd = instance.is_solving_forward ? std::move(forward_mdd) : std::move(backward_mdd);

    if (IS_WRITING_DOT_FILE) {
        write_mdd_dot(*instance.mdd, "initial_filtered.dot");
    }

    const auto flat_mdd_size = calculate_flat_array_size(*instance.mdd);

    instance.shared_object = static_cast<shared_object *>(mmap(
        nullptr, flat_mdd_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    instance.shared_object->is_mdd_reduction_complete = false;
    instance.shared_object->upper_bound = instance.upper_bound;
    instance.shared_object->refinement_round = 1;
    serialize_initial_mdd(*instance.mdd, instance.shared_object);

    while (instance.lower_bound < instance.upper_bound
           && !instance.shared_object->is_mdd_reduction_complete) {
        const double seconds_since_start = get_elapsed_seconds(instance);

        if (seconds_since_start > MDD_TIMEOUT_IN_SECONDS) {
            filter_matches_by_flat_mdd(instance);
            return;
        }

        std::cout << "Elapsed time: " << std::fixed << std::setprecision(2) << seconds_since_start << "s. "
                << "Starting mdd reduction." << std::endl;

        handle_threads_for_mdd_reduction(instance);
        instance.shared_object->refinement_round++;
        instance.upper_bound = std::min(instance.upper_bound, instance.shared_object->upper_bound);
        instance.upper_bound = std::max(instance.upper_bound, instance.lower_bound);
    }
    instance.shared_object->is_mdd_reduction_complete = true;
    filter_matches_by_flat_mdd(instance);
}

void filter_matches_by_flat_mdd(instance &instance) {
    for (auto &match: instance.graph->matches) {
        match.extension.is_active = false;
        match.extension.reversed->extension.is_active = false;
    }
    auto current_pointer = reinterpret_cast<int8_t *>(&instance.shared_object->flat_levels);

    for (size_t level_index = 0; level_index < instance.shared_object->num_levels; ++level_index) {
        const auto *flat_level = reinterpret_cast<struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);

        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = reinterpret_cast<struct flat_node *>(current_pointer);
            auto match_ptr = reinterpret_cast<rflcs_graph::match *>(flat_node->match_ptr);
            match_ptr->extension.is_active = flat_node->is_active;
            match_ptr->extension.reversed->extension.is_active = match_ptr->extension.is_active;
            current_pointer += sizeof(struct flat_node);
            current_pointer += flat_node->num_arcs_out * sizeof(flat_arc);
        }
    }
    instance.graph->matches.front().extension.is_active = false;
    instance.graph->matches.back().extension.is_active = false;
    instance.active_matches = static_cast<int>(std::ranges::count_if(
        instance.graph->matches,
        [](const auto &match) { return match.extension.is_active; }
    ));
}

void handle_threads_for_mdd_reduction(instance &instance) {
    const pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed" << std::endl;
        exit(1);
    }
    if (pid == 0) {
        signal(SIGALRM, timeout_handler);

        itimerval timer{};
        timer.it_value.tv_sec = std::max(1l, MDD_TIMEOUT_IN_SECONDS - static_cast<long>(get_elapsed_seconds(instance)));
        timer.it_value.tv_usec = 0;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;

        setitimer(ITIMER_REAL, &timer, nullptr);

        set_oom_score_adj(1000);

#ifdef __linux__
        // For Linux
        prctl(PR_SET_NAME, "rflcs_mdd", 0, 0, 0);
#elif defined(__APPLE__)
        // For macOS
        pthread_setname_np("rflcs_mdd");
#endif

        if (instance.lower_bound >= instance.upper_bound) {
            return;
        }
        reduce_by_mdd(instance);
        exit(0);
    }
    set_oom_score_adj(-1000);
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        auto usage = rusage();
        if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
            instance.mdd_memory_consumption = std::max(instance.mdd_memory_consumption, sanitize_memory_usage(usage.ru_maxrss));
        } else {
            instance.mdd_memory_consumption = -1;
        }
    } else {
        instance.mdd_memory_consumption = -1;
    }

    instance.upper_bound = std::min(instance.upper_bound, instance.shared_object->upper_bound);
    instance.upper_bound = std::max(instance.upper_bound, instance.lower_bound);
    if (instance.lower_bound >= instance.upper_bound) {
        return;
    }
    prune_by_flat_mdd(instance.shared_object, *instance.mdd, *instance.mdd_node_source);
    filter_mdd(instance, *instance.mdd, true, *instance.mdd_node_source);
    serialize_initial_mdd(*instance.mdd, instance.shared_object);
}

void timeout_handler(const int signal) {
    std::cout << "Child process timed out!\n";
    exit(signal);
}

void set_oom_score_adj(const int score) {
    std::ofstream oom_file;
    oom_file.open("/proc/self/oom_score_adj");
    if (!oom_file) {
        return;
    }
    oom_file << score;
    oom_file.close();
}

double get_elapsed_seconds(const instance &instance) {
    const std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - instance.start;
    return elapsed_seconds.count();
}

long calculate_mdd_complexity(const mdd &mdd) {
    auto counter_set = absl::flat_hash_set<rflcs_graph::match*>();
    for (const auto &level: *mdd.levels) {
        for (const auto node: *level->nodes) {
            counter_set.insert(node->match);
        }
    }
    return static_cast<long>(counter_set.size());
}
