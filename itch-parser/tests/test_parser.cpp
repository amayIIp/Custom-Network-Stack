#include "../include/itch_messages.hpp"
#include "../include/itch_parser.hpp"
#include "../include/byteswap.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

struct MockHandler {
    int s_count = 0;
    int a_count = 0;
    int e_count = 0;
    int x_count = 0;
    int d_count = 0;
    int p_count = 0;
    int other_count = 0;

    itch::SystemEventMsg last_s;
    itch::AddOrderMsg last_a;
    itch::OrderExecutedMsg last_e;
    itch::OrderCancelMsg last_x;
    itch::OrderDeleteMsg last_d;
    itch::TradeMsg last_p;

    void on_system_event(const itch::SystemEventMsg& msg) {
        s_count++;
        last_s = msg;
    }
    void on_add_order(const itch::AddOrderMsg& msg) {
        a_count++;
        last_a = msg;
    }
    void on_order_executed(const itch::OrderExecutedMsg& msg) {
        e_count++;
        last_e = msg;
    }
    void on_order_cancel(const itch::OrderCancelMsg& msg) {
        x_count++;
        last_x = msg;
    }
    void on_order_delete(const itch::OrderDeleteMsg& msg) {
        d_count++;
        last_d = msg;
    }
    void on_trade(const itch::TradeMsg& msg) {
        p_count++;
        last_p = msg;
    }
    void on_other(char type, const uint8_t* msg, uint16_t len) {
        other_count++;
    }
};

// Helper to push big endian uint16_t length prefix
void push_length(std::vector<uint8_t>& buf, uint16_t len) {
    uint16_t be = itch::be16(len);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&be);
    buf.push_back(p[0]);
    buf.push_back(p[1]);
}

void test_system_event() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::SystemEventMsg));
    
    itch::SystemEventMsg msg;
    msg.type = 'S';
    msg.stock_locate = itch::be16(1234);
    msg.tracking_number = itch::be16(5678);
    // Timestamp: 0x010203040506
    msg.timestamp[0] = 0x01; msg.timestamp[1] = 0x02; msg.timestamp[2] = 0x03;
    msg.timestamp[3] = 0x04; msg.timestamp[4] = 0x05; msg.timestamp[5] = 0x06;
    msg.event_code = 'O';
    
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::SystemEventMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.s_count == 1);
    assert(itch::be16(handler.last_s.stock_locate) == 1234);
    assert(itch::be16(handler.last_s.tracking_number) == 5678);
    assert(itch::read_ts48(handler.last_s.timestamp) == 0x010203040506ULL);
    assert(handler.last_s.event_code == 'O');

    MockHandler table_handler;
    itch::TableParser<MockHandler> parser;
    parser.parse(buf.data(), buf.size(), table_handler);
    assert(table_handler.s_count == 1);
    assert(itch::be16(table_handler.last_s.stock_locate) == 1234);
    assert(itch::be16(table_handler.last_s.tracking_number) == 5678);
    assert(itch::read_ts48(table_handler.last_s.timestamp) == 0x010203040506ULL);
    assert(table_handler.last_s.event_code == 'O');
    
    std::cout << "test_system_event passed.\n";
}

void test_add_order() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::AddOrderMsg));

    itch::AddOrderMsg msg;
    msg.type = 'A';
    msg.stock_locate = itch::be16(11);
    msg.tracking_number = itch::be16(22);
    msg.timestamp[0] = 0; msg.timestamp[1] = 0; msg.timestamp[2] = 0;
    msg.timestamp[3] = 0; msg.timestamp[4] = 0; msg.timestamp[5] = 99;
    msg.order_ref = itch::be64(9876543210ULL);
    msg.side = 'B';
    msg.shares = itch::be32(500);
    std::memcpy(msg.stock, "AAPL    ", 8);
    msg.price = itch::be32(1502500); // 150.25

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::AddOrderMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.a_count == 1);
    assert(itch::be16(handler.last_a.stock_locate) == 11);
    assert(itch::be16(handler.last_a.tracking_number) == 22);
    assert(itch::read_ts48(handler.last_a.timestamp) == 99);
    assert(itch::be64(handler.last_a.order_ref) == 9876543210ULL);
    assert(handler.last_a.side == 'B');
    assert(itch::be32(handler.last_a.shares) == 500);
    assert(std::memcmp(handler.last_a.stock, "AAPL    ", 8) == 0);
    assert(itch::be32(handler.last_a.price) == 1502500);

    MockHandler table_handler;
    itch::TableParser<MockHandler> parser;
    parser.parse(buf.data(), buf.size(), table_handler);
    assert(table_handler.a_count == 1);
    assert(itch::be16(table_handler.last_a.stock_locate) == 11);
    assert(itch::be64(table_handler.last_a.order_ref) == 9876543210ULL);
    assert(itch::be32(table_handler.last_a.price) == 1502500);

    std::cout << "test_add_order passed.\n";
}

void test_order_executed() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::OrderExecutedMsg));

    itch::OrderExecutedMsg msg;
    msg.type = 'E';
    msg.stock_locate = itch::be16(1);
    msg.tracking_number = itch::be16(2);
    std::memset(msg.timestamp, 0, 6);
    msg.order_ref = itch::be64(1001ULL);
    msg.executed_shares = itch::be32(100);
    msg.match_number = itch::be64(99999ULL);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::OrderExecutedMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.e_count == 1);
    assert(itch::be64(handler.last_e.order_ref) == 1001ULL);
    assert(itch::be32(handler.last_e.executed_shares) == 100);
    assert(itch::be64(handler.last_e.match_number) == 99999ULL);

    std::cout << "test_order_executed passed.\n";
}

void test_order_cancel() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::OrderCancelMsg));

    itch::OrderCancelMsg msg;
    msg.type = 'X';
    msg.stock_locate = itch::be16(1);
    msg.tracking_number = itch::be16(2);
    std::memset(msg.timestamp, 0, 6);
    msg.order_ref = itch::be64(1001ULL);
    msg.cancelled_shares = itch::be32(50);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::OrderCancelMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.x_count == 1);
    assert(itch::be64(handler.last_x.order_ref) == 1001ULL);
    assert(itch::be32(handler.last_x.cancelled_shares) == 50);

    std::cout << "test_order_cancel passed.\n";
}

void test_order_delete() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::OrderDeleteMsg));

    itch::OrderDeleteMsg msg;
    msg.type = 'D';
    msg.stock_locate = itch::be16(1);
    msg.tracking_number = itch::be16(2);
    std::memset(msg.timestamp, 0, 6);
    msg.order_ref = itch::be64(1001ULL);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::OrderDeleteMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.d_count == 1);
    assert(itch::be64(handler.last_d.order_ref) == 1001ULL);

    std::cout << "test_order_delete passed.\n";
}

void test_trade() {
    std::vector<uint8_t> buf;
    push_length(buf, sizeof(itch::TradeMsg));

    itch::TradeMsg msg;
    msg.type = 'P';
    msg.stock_locate = itch::be16(9);
    msg.tracking_number = itch::be16(8);
    std::memset(msg.timestamp, 0, 6);
    msg.order_ref = itch::be64(0);
    msg.side = 'B';
    msg.shares = itch::be32(2500);
    std::memcpy(msg.stock, "MSFT    ", 8);
    msg.price = itch::be32(2405000);
    msg.match_number = itch::be64(888888ULL);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&msg);
    buf.insert(buf.end(), ptr, ptr + sizeof(itch::TradeMsg));

    MockHandler handler;
    itch::parse_itch_switch(buf.data(), buf.size(), handler);
    assert(handler.p_count == 1);
    assert(itch::be16(handler.last_p.stock_locate) == 9);
    assert(itch::be32(handler.last_p.shares) == 2500);
    assert(std::memcmp(handler.last_p.stock, "MSFT    ", 8) == 0);
    assert(itch::be32(handler.last_p.price) == 2405000);
    assert(itch::be64(handler.last_p.match_number) == 888888ULL);

    std::cout << "test_trade passed.\n";
}

void test_garbage_and_bounds() {
    // 1. Truncated buffer (only length prefix, no message payload)
    std::vector<uint8_t> buf1;
    push_length(buf1, 10); // claims length is 10, but buffer has no payload
    MockHandler handler1;
    itch::parse_itch_switch(buf1.data(), buf1.size(), handler1);
    assert(handler1.s_count == 0 && handler1.other_count == 0);

    // 2. Truncated message inside buffer
    std::vector<uint8_t> buf2;
    push_length(buf2, sizeof(itch::SystemEventMsg));
    itch::SystemEventMsg s_msg;
    s_msg.type = 'S';
    s_msg.stock_locate = itch::be16(1);
    s_msg.tracking_number = itch::be16(2);
    std::memset(s_msg.timestamp, 0, 6);
    s_msg.event_code = 'O';
    // Append only half of it
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&s_msg);
    buf2.insert(buf2.end(), ptr, ptr + 5); // Insert only 5 bytes of the 12-byte payload
    
    MockHandler handler2;
    itch::parse_itch_switch(buf2.data(), buf2.size(), handler2);
    assert(handler2.s_count == 0);

    // 3. Other/Unknown message type handling
    std::vector<uint8_t> buf3;
    push_length(buf3, 5);
    buf3.push_back('Z'); // Unknown message type
    buf3.push_back(1);
    buf3.push_back(2);
    buf3.push_back(3);
    buf3.push_back(4);

    MockHandler handler3;
    itch::parse_itch_switch(buf3.data(), buf3.size(), handler3);
    assert(handler3.other_count == 1);

    std::cout << "test_garbage_and_bounds passed.\n";
}

int main() {
    std::cout << "Running Parser Unit Tests...\n";
    test_system_event();
    test_add_order();
    test_order_executed();
    test_order_cancel();
    test_order_delete();
    test_trade();
    test_garbage_and_bounds();
    std::cout << "All Unit Tests Passed Successfully!\n";
    return 0;
}
