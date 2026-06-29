#include "../include/parser.hpp"
#include "../bench/naive_parser.hpp"
#include "../include/order_book.hpp"
#include "../include/mmap_file.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <stdexcept>

void print_stats(const itch::ParserStats& stats, double elapsed_seconds) {
    std::cout << "-------------------------------------------\n";
    std::cout << "Parse Completed in: " << elapsed_seconds << " seconds\n";
    std::cout << "Total Messages:     " << stats.total_messages << "\n";
    std::cout << "  S (System Event): " << stats.msg_count_S << "\n";
    std::cout << "  A (Add Order):    " << stats.msg_count_A << "\n";
    std::cout << "  E (Executed):     " << stats.msg_count_E << "\n";
    std::cout << "  X (Cancel):       " << stats.msg_count_X << "\n";
    std::cout << "  D (Delete):       " << stats.msg_count_D << "\n";
    std::cout << "  P (Trade):        " << stats.msg_count_P << "\n";
    std::cout << "  Other msgs:       " << stats.msg_count_other << "\n";
    std::cout << "Shares added:       " << stats.total_shares_added << "\n";
    std::cout << "Shares executed:    " << stats.total_shares_executed << "\n";
    std::cout << "Shares cancelled:   " << stats.total_shares_cancelled << "\n";
    std::cout << "Shares traded:      " << stats.total_shares_traded << "\n";
    std::cout << "Min Price:          " << stats.min_price << "\n";
    std::cout << "Max Price:          " << stats.max_price << "\n";
    if (stats.total_messages > 0) {
        double speed = stats.total_messages / elapsed_seconds / 1e6;
        double ns_per_msg = (elapsed_seconds * 1e9) / stats.total_messages;
        std::cout << "Throughput:         " << speed << " million msgs/sec\n";
        std::cout << "Avg Latency:        " << ns_per_msg << " ns/msg\n";
    }
    std::cout << "-------------------------------------------\n";
}

void run_order_book_parser(const std::string& filepath) {
    itch::MemoryMappedFile file;
    if (!file.open(filepath)) {
        std::cerr << "Failed to open file: " << filepath << "\n";
        return;
    }
    
    std::cout << "Mapping file " << filepath << " and starting Order Book parser...\n";
    
    auto start = std::chrono::steady_clock::now();
    itch::OrderBookHandler handler;
    itch::TableParser<itch::OrderBookHandler> parser;
    parser.parse(file.data(), file.size(), handler);
    auto end = std::chrono::steady_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Order Book Parser stats:\n";
    std::cout << "  Time Taken:          " << elapsed.count() << " seconds\n";
    std::cout << "  Total Messages:      " << handler.total_messages << "\n";
    std::cout << "  Active Orders:       " << handler.orders.size() << "\n";
    std::cout << "  Active Stocks tracked: " << handler.stock_books.size() << "\n";
    std::cout << "  Bid/Ask Cross Events: " << handler.bid_ask_cross_events << "\n";
    if (handler.total_messages > 0) {
        double speed = handler.total_messages / elapsed.count() / 1e6;
        double ns_per_msg = (elapsed.count() * 1e9) / handler.total_messages;
        std::cout << "  Throughput:          " << speed << " million msgs/sec\n";
        std::cout << "  Avg Latency:         " << ns_per_msg << " ns/msg\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <mode: naive|switch|table|book> <filepath>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string filepath = argv[2];

    try {
        if (mode == "naive") {
            std::cout << "Running naive baseline parser...\n";
            auto start = std::chrono::steady_clock::now();
            itch::ParserStats stats = itch::run_naive_parser(filepath);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            print_stats(stats, elapsed.count());
        } else if (mode == "switch") {
            std::cout << "Running optimized switch-based parser...\n";
            auto start = std::chrono::steady_clock::now();
            itch::ParserStats stats = itch::run_switch_parser(filepath);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            print_stats(stats, elapsed.count());
        } else if (mode == "table") {
            std::cout << "Running optimized table-based (jump-table) parser...\n";
            auto start = std::chrono::steady_clock::now();
            itch::ParserStats stats = itch::run_table_parser(filepath);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            print_stats(stats, elapsed.count());
        } else if (mode == "book") {
            run_order_book_parser(filepath);
        } else {
            std::cerr << "Unknown mode: " << mode << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
