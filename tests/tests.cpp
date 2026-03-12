#include <catch2/catch_all.hpp> 
#include "../include/orderbook.h"

// helper to build a limit order cleanly
Order makeLimit(int id, OrderSide side, double price, int64_t qty) {
    return Order(id, side, toTicks(price), qty, OrderType::LIMIT);
}

Order makeMarket(int id, OrderSide side, int64_t qty) {
    return Order(id, side, 0, qty, OrderType::MARKET);
}

// ── Basic matching ────────────────────────────────────────────────────────────

TEST_CASE("Matching buy and sell at same price creates a trade") {
    OrderBook ob;
    ob.addOrder(makeLimit(1, OrderSide::BUY,  100.0, 10));
    ob.addOrder(makeLimit(2, OrderSide::SELL, 100.0, 10));

    REQUIRE(ob.getBestBid() == 0.0); // fully filled — nothing resting
    REQUIRE(ob.getBestAsk() == 0.0);
}

TEST_CASE("No match when spread exists") {
    OrderBook ob;
    ob.addOrder(makeLimit(1, OrderSide::BUY,  99.0, 10));
    ob.addOrder(makeLimit(2, OrderSide::SELL, 101.0, 10));

    REQUIRE(ob.getBestBid() == 99.0);
    REQUIRE(ob.getBestAsk() == 101.0);
}

TEST_CASE("Partial fill leaves residual resting") {
    OrderBook ob;
    ob.addOrder(makeLimit(1, OrderSide::BUY,  100.0, 10));
    ob.addOrder(makeLimit(2, OrderSide::SELL, 100.0,  4));

    // sell only filled 4 — 6 should remain on bid
    REQUIRE(ob.getBestBid() == 100.0);
    REQUIRE(ob.getBestAsk() == 0.0);
}

// ── Cancel ────────────────────────────────────────────────────────────────────

TEST_CASE("Cancel removes order from book") {
    OrderBook ob;
    ob.addOrder(makeLimit(1, OrderSide::BUY, 100.0, 10));
    ob.cancelOrder(1);

    REQUIRE(ob.getBestBid() == 0.0);
}

TEST_CASE("Cancel non-existent order returns false") {
    OrderBook ob;
    REQUIRE(ob.cancelOrder(99) == false);
}

// ── Market orders ─────────────────────────────────────────────────────────────

TEST_CASE("Market buy fills against best ask") {
    OrderBook ob;
    ob.addOrder(makeLimit(1, OrderSide::SELL, 101.0, 10));
    ob.addOrder(makeMarket(2, OrderSide::BUY, 10));

    REQUIRE(ob.getBestAsk() == 0.0); // fully consumed
}

TEST_CASE("Market order with no liquidity does nothing") {
    OrderBook ob;
    ob.addOrder(makeMarket(1, OrderSide::BUY, 10)); // empty book — should not crash
    REQUIRE(ob.getBestBid() == 0.0);
    REQUIRE(ob.getBestAsk() == 0.0);
}