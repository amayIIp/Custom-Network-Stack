cmake_minimum_required(VERSION 3.15)
project(itch-parser CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure compiler flags for maximum optimization on hot path
if(MSVC)
    add_compile_options(/O2 /GL /Gw /arch:AVX2 /W4)
    add_link_options(/LTCG)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-O3 -march=native -flto -Wall -Wextra -Wpedantic)
    add_link_options(-flto)
endif()

# Include directories
include_directories(include)

# Define shared library/sources
add_library(parser_lib STATIC
    src/parser.cpp
    bench/naive_parser.cpp
)

# Main CLI Executable
add_executable(itch_parser
    src/main.cpp
)
target_link_libraries(itch_parser PRIVATE parser_lib)

# Benchmarks executable
add_executable(bench_parser
    bench/bench_parser.cpp
)
target_link_libraries(bench_parser PRIVATE parser_lib)

# Unit Tests executable
add_executable(test_parser
    tests/test_parser.cpp
)
target_link_libraries(test_parser PRIVATE parser_lib)

# Enable CTest
enable_testing()
add_test(NAME test_parser COMMAND test_parser)
