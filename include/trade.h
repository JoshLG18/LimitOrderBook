#ifndef TRADE_H
#define TRADE_H

// creating a type to track the info about a trade
struct Trade {
    int buyOrderID;
    int sellOrderID;
    int64_t quantity;
    int64_t price;
    int64_t latencyNs;
    std::chrono::nanoseconds timestamp; // when the trade is made
};

#endif
