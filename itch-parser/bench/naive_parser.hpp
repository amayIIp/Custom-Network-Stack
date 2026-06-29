#pragma once
#include "../include/itch_parser.hpp"
#include <string>

namespace itch {

// Run the naive parser on the given file path and return the statistics
ParserStats run_naive_parser(const std::string& filepath);

} // namespace itch
