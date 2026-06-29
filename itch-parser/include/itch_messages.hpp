#pragma once
#include <cstdint>

#pragma pack(push, 1)

namespace itch {

// Message 'S': System Event Message (12 bytes)
struct SystemEventMsg {
    char type;                // 'S'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char event_code;
};
static_assert(sizeof(SystemEventMsg) == 12, "SystemEventMsg size mismatch");

// Message 'A': Add Order - No MPID Attribution (36 bytes)
struct AddOrderMsg {
    char type;                // 'A'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref;
    char side;                // 'B' or 'S'
    uint32_t shares;
    char stock[8];
    uint32_t price;
};
static_assert(sizeof(AddOrderMsg) == 36, "AddOrderMsg size mismatch");

// Message 'E': Order Executed (30 bytes)
struct OrderExecutedMsg {
    char type;                // 'E'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref;
    uint32_t executed_shares;
    uint64_t match_number;
};
static_assert(sizeof(OrderExecutedMsg) == 31, "OrderExecutedMsg size mismatch");

// Message 'X': Order Cancel (22 bytes)
struct OrderCancelMsg {
    char type;                // 'X'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref;
    uint32_t cancelled_shares;
};
static_assert(sizeof(OrderCancelMsg) == 23, "OrderCancelMsg size mismatch");

// Message 'D': Order Delete (18 bytes)
struct OrderDeleteMsg {
    char type;                // 'D'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref;
};
static_assert(sizeof(OrderDeleteMsg) == 19, "OrderDeleteMsg size mismatch");

// Message 'P': Trade Message (Non-Cross) (44 bytes)
struct TradeMsg {
    char type;                // 'P'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref;
    char side;                // 'B' or 'S' (always 'B' after 2014)
    uint32_t shares;
    char stock[8];
    uint32_t price;
    uint64_t match_number;
};
static_assert(sizeof(TradeMsg) == 44, "TradeMsg size mismatch");

} // namespace itch

#pragma pack(pop)
