cmake_minimum_required(VERSION 3.22.1)
project(rflcs)
set(CMAKE_CXX_STANDARD 23)

add_compile_options(-O3)
add_compile_options(-march=native)
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-Wno-unknown-pragmas)
add_compile_options(-Wno-c99-extensions)
add_compile_options(-Wno-flexible-array-extensions)
add_compile_options(-Wno-nullability-extension)
add_compile_options(-Wno-strict-prototypes)
add_compile_options(-Wno-gcc-compat)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-unused-but-set-variable)
add_compile_options(-Wno-unused-variable)

add_library(boost_timer INTERFACE)
add_library(boost_dynamic_bitset INTERFACE)
target_include_directories(boost_dynamic_bitset INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/container_hash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/describe/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/dynamic_bitset/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/integer/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/move/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/mp11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/static_assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/throw_exception/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/type_traits/include
)

target_include_directories(boost_timer INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/libs/timer/include
)


add_subdirectory(external/abseil)

if(DEFINED ENV{GUROBI_HOME})
    set(GUROBI_HOME $ENV{GUROBI_HOME})
    set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include")

    find_library(GUROBI_LIBRARY
            NAMES gurobi gurobi100 gurobi110 gurobi120 gurobi130
            HINTS "${GUROBI_HOME}/lib"
    )
    find_library(GUROBI_API
            NAMES libgurobi_c++.a
            HINTS "${GUROBI_HOME}/lib"
    )
else()
    message(FATAL_ERROR "GUROBI_HOME environment variable is not set")
endif()

set(SOURCE_FILES
        src/formatting_utils.cpp
        src/globals.cpp
        src/graph/rf_subset_lcs_relaxation.cpp
        src/graph/graph_creation.cpp
        src/graph/graph_writer.cpp
        src/graph/match_deactivation.cpp
        src/graph/match_metrics.cpp
        src/graph/reduce_graph.cpp
        src/graph/simple_upper_bounds.cpp
        src/heuristic.cpp
        src/ilp_solver/gurobi_graph_ilp.cpp
        src/main.cpp
        src/mdd/character_bound_utils.cpp
        src/mdd/character_selection.cpp
        src/mdd/domination_utils.cpp
        src/mdd/flat_mdd.cpp
        src/mdd/initial_mdd.cpp
        src/mdd/mdd_dot_writer.cpp
        src/mdd/mdd_filter.cpp
        src/mdd/mdd_reduction.cpp
        src/mdd/mdd_refinement.cpp
        src/mdd/edge_utils.cpp
        src/reduction_orchestration.cpp
        src/result_writer.cpp
        src/solver/sequence_enumeration_solver.cpp
        src/utils.cpp
)

add_executable(rflcs ${SOURCE_FILES})

target_include_directories(rflcs PRIVATE ${GUROBI_INCLUDE_DIR})

target_link_libraries(rflcs
        absl::flat_hash_set
        absl::flat_hash_map
        boost_dynamic_bitset
        boost_timer
        ${GUROBI_LIBRARY}
        ${GUROBI_API}
)
