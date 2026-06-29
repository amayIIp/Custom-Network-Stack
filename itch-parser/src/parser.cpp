#include "../include/parser.hpp"
#include "../include/mmap_file.hpp"
#include <stdexcept>

namespace itch {

ParserStats run_switch_parser(const std::string& filepath) {
    MemoryMappedFile file;
    if (!file.open(filepath)) {
        throw std::runtime_error("Failed to map file: " + filepath);
    }
    
    StatsHandler handler;
    parse_itch_switch(file.data(), file.size(), handler);
    return handler.stats;
}

ParserStats run_table_parser(const std::string& filepath) {
    MemoryMappedFile file;
    if (!file.open(filepath)) {
        throw std::runtime_error("Failed to map file: " + filepath);
    }
    
    StatsHandler handler;
    TableParser<StatsHandler> parser;
    parser.parse(file.data(), file.size(), handler);
    return handler.stats;
}

} // namespace itch
