#pragma once
#include "itch_parser.hpp"
#include <string>

namespace itch {

ParserStats run_switch_parser(const std::string& filepath);
ParserStats run_table_parser(const std::string& filepath);

} // namespace itch
