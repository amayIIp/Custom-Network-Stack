#include "naive_parser.hpp"
#include <fstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace itch {

ParserStats run_naive_parser(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file in naive parser: " + filepath);
    }

    ParserStats stats;

    // Dispatch table using std::unordered_map and std::function
    std::unordered_map<char, std::function<void(const std::vector<uint8_t>&)>> dispatch_map;

    dispatch_map['S'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_S++;
    };

    dispatch_map['A'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_A++;
        if (msg.size() >= sizeof(AddOrderMsg)) {
            const auto* add_msg = reinterpret_cast<const AddOrderMsg*>(msg.data());
            uint32_t shares = be32(add_msg->shares);
            stats.total_shares_added += shares;
            
            // Allocation: Copy stock to std::string
            std::string stock_symbol(add_msg->stock, 8);
            
            uint32_t price = be32(add_msg->price);
            if (price > 0) {
                stats.min_price = std::min(stats.min_price, price);
                stats.max_price = std::max(stats.max_price, price);
            }
        }
    };

    dispatch_map['E'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_E++;
        if (msg.size() >= sizeof(OrderExecutedMsg)) {
            const auto* exec_msg = reinterpret_cast<const OrderExecutedMsg*>(msg.data());
            stats.total_shares_executed += be32(exec_msg->executed_shares);
        }
    };

    dispatch_map['X'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_X++;
        if (msg.size() >= sizeof(OrderCancelMsg)) {
            const auto* cancel_msg = reinterpret_cast<const OrderCancelMsg*>(msg.data());
            stats.total_shares_cancelled += be32(cancel_msg->cancelled_shares);
        }
    };

    dispatch_map['D'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_D++;
    };

    dispatch_map['P'] = [&](const std::vector<uint8_t>& msg) {
        stats.total_messages++;
        stats.msg_count_P++;
        if (msg.size() >= sizeof(TradeMsg)) {
            const auto* trade_msg = reinterpret_cast<const TradeMsg*>(msg.data());
            stats.total_shares_traded += be32(trade_msg->shares);
            
            // Allocation: Copy stock to std::string
            std::string stock_symbol(trade_msg->stock, 8);
            
            uint32_t price = be32(trade_msg->price);
            if (price > 0) {
                stats.min_price = std::min(stats.min_price, price);
                stats.max_price = std::max(stats.max_price, price);
            }
        }
    };

    // Buffer for reading length prefix
    char len_buf[2];

    while (file.read(len_buf, 2)) {
        uint16_t msg_len = be16(*reinterpret_cast<const uint16_t*>(len_buf));
        
        // Heap allocation of the message payload vector
        std::vector<uint8_t> msg(msg_len);
        if (!file.read(reinterpret_cast<char*>(msg.data()), msg_len)) {
            break; // Truncated or EOF
        }

        char type = static_cast<char>(msg[0]);
        auto it = dispatch_map.find(type);
        if (it != dispatch_map.end()) {
            it->second(msg);
        } else {
            stats.total_messages++;
            stats.msg_count_other++;
        }
    }

    return stats;
}

} // namespace itch
