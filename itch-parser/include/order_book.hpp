#pragma once
#include "itch_messages.hpp"
#include "byteswap.hpp"
#include <unordered_map>
#include <map>
#include <cstdint>
#include <algorithm>

namespace itch {

struct Order {
    uint64_t order_ref;
    uint32_t shares;
    uint32_t price;
    uint16_t stock_locate;
    char side; // 'B' or 'S'
};

class OrderBookHandler {
public:
    // Active orders: order_ref -> Order
    std::unordered_map<uint64_t, Order> orders;

    // Track active prices per stock locate for bid/ask tracking
    struct PriceLevelBook {
        std::map<uint32_t, uint64_t> buy_prices;  // price -> cumulative shares
        std::map<uint32_t, uint64_t> sell_prices; // price -> cumulative shares
    };
    
    std::unordered_map<uint16_t, PriceLevelBook> stock_books;

    uint64_t total_messages = 0;
    uint64_t bid_ask_cross_events = 0; // Number of times bid >= ask is observed

    inline void add_price(uint16_t locate, char side, uint32_t price, uint32_t shares) {
        auto& book = stock_books[locate];
        if (side == 'B') {
            book.buy_prices[price] += shares;
        } else {
            book.sell_prices[price] += shares;
        }
    }

    inline void remove_price(uint16_t locate, char side, uint32_t price, uint32_t shares) {
        auto& book = stock_books[locate];
        if (side == 'B') {
            auto it = book.buy_prices.find(price);
            if (it != book.buy_prices.end()) {
                if (it->second <= shares) {
                    book.buy_prices.erase(it);
                } else {
                    it->second -= shares;
                }
            }
        } else {
            auto it = book.sell_prices.find(price);
            if (it != book.sell_prices.end()) {
                if (it->second <= shares) {
                    book.sell_prices.erase(it);
                } else {
                    it->second -= shares;
                }
            }
        }
    }

    inline void check_cross(uint16_t locate) {
        auto it = stock_books.find(locate);
        if (it == stock_books.end()) return;
        const auto& book = it->second;
        if (!book.buy_prices.empty() && !book.sell_prices.empty()) {
            uint32_t best_bid = book.buy_prices.rbegin()->first;
            uint32_t best_ask = book.sell_prices.begin()->first;
            if (best_bid >= best_ask) {
                bid_ask_cross_events++;
            }
        }
    }

    inline void on_system_event(const SystemEventMsg&) {
        total_messages++;
    }

    inline void on_add_order(const AddOrderMsg& msg) {
        total_messages++;
        uint64_t ref = be64(msg.order_ref);
        uint32_t shares = be32(msg.shares);
        uint32_t price = be32(msg.price);
        uint16_t locate = be16(msg.stock_locate);
        char side = msg.side;

        // Insert into orders
        orders[ref] = Order{ref, shares, price, locate, side};

        // Update price book
        add_price(locate, side, price, shares);
        
        // Check for bid/ask cross
        check_cross(locate);
    }

    inline void on_order_executed(const OrderExecutedMsg& msg) {
        total_messages++;
        uint64_t ref = be64(msg.order_ref);
        uint32_t exec_shares = be32(msg.executed_shares);

        auto it = orders.find(ref);
        if (it != orders.end()) {
            Order& ord = it->second;
            remove_price(ord.stock_locate, ord.side, ord.price, exec_shares);
            if (ord.shares <= exec_shares) {
                orders.erase(it);
            } else {
                ord.shares -= exec_shares;
            }
            check_cross(ord.stock_locate);
        }
    }

    inline void on_order_cancel(const OrderCancelMsg& msg) {
        total_messages++;
        uint64_t ref = be64(msg.order_ref);
        uint32_t cancel_shares = be32(msg.cancelled_shares);

        auto it = orders.find(ref);
        if (it != orders.end()) {
            Order& ord = it->second;
            remove_price(ord.stock_locate, ord.side, ord.price, cancel_shares);
            if (ord.shares <= cancel_shares) {
                orders.erase(it);
            } else {
                ord.shares -= cancel_shares;
            }
            check_cross(ord.stock_locate);
        }
    }

    inline void on_order_delete(const OrderDeleteMsg& msg) {
        total_messages++;
        uint64_t ref = be64(msg.order_ref);

        auto it = orders.find(ref);
        if (it != orders.end()) {
            Order& ord = it->second;
            remove_price(ord.stock_locate, ord.side, ord.price, ord.shares);
            orders.erase(it);
            check_cross(ord.stock_locate);
        }
    }

    inline void on_trade(const TradeMsg&) {
        total_messages++;
    }

    inline void on_other(char, const uint8_t*, uint16_t) {
        total_messages++;
    }
};

} // namespace itch
