#pragma once
#include "itch_messages.hpp"
#include "byteswap.hpp"
#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace itch {

struct ParserStats {
    uint64_t total_messages = 0;
    uint64_t msg_count_S = 0;
    uint64_t msg_count_A = 0;
    uint64_t msg_count_E = 0;
    uint64_t msg_count_X = 0;
    uint64_t msg_count_D = 0;
    uint64_t msg_count_P = 0;
    uint64_t msg_count_other = 0;

    uint64_t total_shares_added = 0;
    uint64_t total_shares_executed = 0;
    uint64_t total_shares_cancelled = 0;
    uint64_t total_shares_traded = 0;

    uint32_t min_price = 0xFFFFFFFF;
    uint32_t max_price = 0;
};

// Default handler to collect parser stats
class StatsHandler {
public:
    ParserStats stats;

    inline void on_system_event(const SystemEventMsg&) {
        stats.total_messages++;
        stats.msg_count_S++;
    }

    inline void on_add_order(const AddOrderMsg& msg) {
        stats.total_messages++;
        stats.msg_count_A++;
        uint32_t shares = be32(msg.shares);
        stats.total_shares_added += shares;
        uint32_t price = be32(msg.price);
        if (price > 0) {
            stats.min_price = std::min(stats.min_price, price);
            stats.max_price = std::max(stats.max_price, price);
        }
    }

    inline void on_order_executed(const OrderExecutedMsg& msg) {
        stats.total_messages++;
        stats.msg_count_E++;
        stats.total_shares_executed += be32(msg.executed_shares);
    }

    inline void on_order_cancel(const OrderCancelMsg& msg) {
        stats.total_messages++;
        stats.msg_count_X++;
        stats.total_shares_cancelled += be32(msg.cancelled_shares);
    }

    inline void on_order_delete(const OrderDeleteMsg&) {
        stats.total_messages++;
        stats.msg_count_D++;
    }

    inline void on_trade(const TradeMsg& msg) {
        stats.total_messages++;
        stats.msg_count_P++;
        stats.total_shares_traded += be32(msg.shares);
        uint32_t price = be32(msg.price);
        if (price > 0) {
            stats.min_price = std::min(stats.min_price, price);
            stats.max_price = std::max(stats.max_price, price);
        }
    }

    inline void on_other(char, const uint8_t*, uint16_t) {
        stats.total_messages++;
        stats.msg_count_other++;
    }
};

// Switch-based parser
template <typename Handler>
inline void parse_itch_switch(const uint8_t* data, size_t len, Handler& handler) {
    size_t off = 0;
    while (off + 2 <= len) {
        uint16_t msg_len = be16(*reinterpret_cast<const uint16_t*>(data + off));
        if (off + 2 + msg_len > len) {
            break;
        }
        const uint8_t* msg = data + off + 2;
        
        // Prefetch next message length & type (2 + msg_len ahead)
#if defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(data + off + 2 + msg_len, 0, 3);
#elif defined(_MSC_VER)
        _mm_prefetch(reinterpret_cast<const char*>(data + off + 2 + msg_len), _MM_HINT_T0);
#endif

        char type = static_cast<char>(msg[0]);
        switch (type) {
            case 'S':
                if (msg_len >= sizeof(SystemEventMsg)) {
                    handler.on_system_event(*reinterpret_cast<const SystemEventMsg*>(msg));
                }
                break;
            case 'A':
                if (msg_len >= sizeof(AddOrderMsg)) {
                    handler.on_add_order(*reinterpret_cast<const AddOrderMsg*>(msg));
                }
                break;
            case 'E':
                if (msg_len >= sizeof(OrderExecutedMsg)) {
                    handler.on_order_executed(*reinterpret_cast<const OrderExecutedMsg*>(msg));
                }
                break;
            case 'X':
                if (msg_len >= sizeof(OrderCancelMsg)) {
                    handler.on_order_cancel(*reinterpret_cast<const OrderCancelMsg*>(msg));
                }
                break;
            case 'D':
                if (msg_len >= sizeof(OrderDeleteMsg)) {
                    handler.on_order_delete(*reinterpret_cast<const OrderDeleteMsg*>(msg));
                }
                break;
            case 'P':
                if (msg_len >= sizeof(TradeMsg)) {
                    handler.on_trade(*reinterpret_cast<const TradeMsg*>(msg));
                }
                break;
            default:
                handler.on_other(type, msg, msg_len);
                break;
        }
        off += 2 + msg_len;
    }
}

// Table-based parser (jump table)
template <typename Handler>
class TableParser {
public:
    using HandlerFunc = void(*)(Handler& handler, const uint8_t* msg, uint16_t msg_len);

private:
    std::array<HandlerFunc, 256> m_table;

    static void handle_S(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(SystemEventMsg)) {
            handler.on_system_event(*reinterpret_cast<const SystemEventMsg*>(msg));
        }
    }

    static void handle_A(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(AddOrderMsg)) {
            handler.on_add_order(*reinterpret_cast<const AddOrderMsg*>(msg));
        }
    }

    static void handle_E(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(OrderExecutedMsg)) {
            handler.on_order_executed(*reinterpret_cast<const OrderExecutedMsg*>(msg));
        }
    }

    static void handle_X(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(OrderCancelMsg)) {
            handler.on_order_cancel(*reinterpret_cast<const OrderCancelMsg*>(msg));
        }
    }

    static void handle_D(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(OrderDeleteMsg)) {
            handler.on_order_delete(*reinterpret_cast<const OrderDeleteMsg*>(msg));
        }
    }

    static void handle_P(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        if (msg_len >= sizeof(TradeMsg)) {
            handler.on_trade(*reinterpret_cast<const TradeMsg*>(msg));
        }
    }

    static void handle_other(Handler& handler, const uint8_t* msg, uint16_t msg_len) {
        handler.on_other(static_cast<char>(msg[0]), msg, msg_len);
    }

public:
    TableParser() {
        m_table.fill(handle_other);
        m_table['S'] = handle_S;
        m_table['A'] = handle_A;
        m_table['E'] = handle_E;
        m_table['X'] = handle_X;
        m_table['D'] = handle_D;
        m_table['P'] = handle_P;
    }

    inline void parse(const uint8_t* data, size_t len, Handler& handler) const {
        size_t off = 0;
        while (off + 2 <= len) {
            uint16_t msg_len = be16(*reinterpret_cast<const uint16_t*>(data + off));
            if (off + 2 + msg_len > len) {
                break;
            }
            const uint8_t* msg = data + off + 2;
            
            // Prefetch next message length & type (2 + msg_len ahead)
#if defined(__GNUC__) || defined(__clang__)
            __builtin_prefetch(data + off + 2 + msg_len, 0, 3);
#elif defined(_MSC_VER)
            _mm_prefetch(reinterpret_cast<const char*>(data + off + 2 + msg_len), _MM_HINT_T0);
#endif

            m_table[msg[0]](handler, msg, msg_len);
            off += 2 + msg_len;
        }
    }
};

} // namespace itch
