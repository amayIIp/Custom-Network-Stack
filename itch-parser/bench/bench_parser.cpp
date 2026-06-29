#include "../include/parser.hpp"
#include "naive_parser.hpp"
#include "../include/mmap_file.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <stdexcept>

// Helper to calculate median
double get_median(std::vector<double>& times) {
    std::sort(times.begin(), times.end());
    size_t size = times.size();
    if (size == 0) return 0.0;
    if (size % 2 == 0) {
        return (times[size / 2 - 1] + times[size / 2]) / 2.0;
    }
    return times[size / 2];
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filepath>\n";
        return 1;
    }
    std::string filepath = argv[1];

    std::cout << "Benchmarking ITCH Parsers on " << filepath << "\n";
    std::cout << "===========================================\n";

    // 1. Benchmark Naive Parser
    try {
        std::cout << "Running Naive Parser (2 warmups + 10 runs)..." << std::endl;
        // Warmup
        for (int i = 0; i < 2; ++i) {
            itch::run_naive_parser(filepath);
        }
        
        std::vector<double> naive_times;
        uint64_t msg_count = 0;
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::steady_clock::now();
            auto stats = itch::run_naive_parser(filepath);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            naive_times.push_back(diff.count());
            msg_count = stats.total_messages;
        }
        
        double median_time = get_median(naive_times);
        double msgs_per_sec = msg_count / median_time;
        double ns_per_msg = (median_time * 1e9) / msg_count;
        std::cout << "Naive Parser Median Time: " << median_time << " s\n"
                  << "  Throughput: " << (msgs_per_sec / 1e6) << " million msgs/sec\n"
                  << "  Latency:    " << ns_per_msg << " ns/msg\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Naive Parser benchmark failed: " << e.what() << "\n\n";
    }

    // 2. Benchmark Switch Parser (in-memory parsing)
    try {
        std::cout << "Running Switch Parser (2 warmups + 10 runs)..." << std::endl;
        itch::MemoryMappedFile file;
        if (!file.open(filepath)) {
            throw std::runtime_error("Failed to map file");
        }
        
        // Warmup
        for (int i = 0; i < 2; ++i) {
            itch::StatsHandler handler;
            itch::parse_itch_switch(file.data(), file.size(), handler);
        }
        
        std::vector<double> switch_times;
        uint64_t msg_count = 0;
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::steady_clock::now();
            itch::StatsHandler handler;
            itch::parse_itch_switch(file.data(), file.size(), handler);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            switch_times.push_back(diff.count());
            msg_count = handler.stats.total_messages;
        }
        
        double median_time = get_median(switch_times);
        double msgs_per_sec = msg_count / median_time;
        double ns_per_msg = (median_time * 1e9) / msg_count;
        std::cout << "Switch Parser Median Time: " << median_time << " s\n"
                  << "  Throughput: " << (msgs_per_sec / 1e6) << " million msgs/sec\n"
                  << "  Latency:    " << ns_per_msg << " ns/msg\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Switch Parser benchmark failed: " << e.what() << "\n\n";
    }

    // 3. Benchmark Table Parser (in-memory parsing)
    try {
        std::cout << "Running Table Parser (2 warmups + 10 runs)..." << std::endl;
        itch::MemoryMappedFile file;
        if (!file.open(filepath)) {
            throw std::runtime_error("Failed to map file");
        }
        itch::TableParser<itch::StatsHandler> parser;
        
        // Warmup
        for (int i = 0; i < 2; ++i) {
            itch::StatsHandler handler;
            parser.parse(file.data(), file.size(), handler);
        }
        
        std::vector<double> table_times;
        uint64_t msg_count = 0;
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::steady_clock::now();
            itch::StatsHandler handler;
            parser.parse(file.data(), file.size(), handler);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            table_times.push_back(diff.count());
            msg_count = handler.stats.total_messages;
        }
        
        double median_time = get_median(table_times);
        double msgs_per_sec = msg_count / median_time;
        double ns_per_msg = (median_time * 1e9) / msg_count;
        std::cout << "Table Parser Median Time: " << median_time << " s\n"
                  << "  Throughput: " << (msgs_per_sec / 1e6) << " million msgs/sec\n"
                  << "  Latency:    " << ns_per_msg << " ns/msg\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Table Parser benchmark failed: " << e.what() << "\n\n";
    }

    return 0;
}
