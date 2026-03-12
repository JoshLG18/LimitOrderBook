#ifndef ANALYTICS_H
#define ANALYTICS_H

#include "trade.h"
#include "order.h"
#include <iostream>

class Analytics {
public:
    int64_t totalTrades = 0;
    int64_t totalVolume = 0;
    int64_t buyOrders = 0;
    int64_t sellOrders = 0;
    int64_t totalLatencyNs = 0;
    int64_t totalValueTicks = 0; // sum of price * quantity in ticks

    void recordTrade(const Trade& trade) {
        totalTrades++;
        totalVolume += trade.quantity;
        totalLatencyNs += trade.latencyNs;
        totalValueTicks += trade.price * trade.quantity;
    }

    void recordOrder(OrderSide side) {
        if (side == OrderSide::BUY) buyOrders++;
        else sellOrders++;
    }

    double getSpread(double bestBid, double bestAsk) const {
        if (bestBid == 0.0 || bestAsk == 0.0) return 0.0;
        return bestAsk - bestBid;
    }

    double getVWAP() const {
        if (totalVolume == 0) return 0.0;
        return fromTicks(totalValueTicks / totalVolume);
    }

    double getAvgLatencyUs() const {
        if (totalTrades == 0) return 0.0;
        return (totalLatencyNs / totalTrades) / 1000.0; // convert to microseconds
    }

    void displaySummary(double bestBid, double bestAsk) const {
        std::cout << "\nSummary: " << std::endl;
        std::cout << "-------------------------" << std::endl;
        std::cout << "Total Trades:     " << totalTrades << std::endl;
        std::cout << "Total Volume:     " << totalVolume << std::endl;
        std::cout << "Buy Orders:       " << buyOrders << std::endl;
        std::cout << "Sell Orders:      " << sellOrders << std::endl;
        std::cout << "VWAP:             " << getVWAP() << std::endl;
        std::cout << "Avg Latency:      " << getAvgLatencyUs() << " us" << std::endl;
        std::cout << "Bid-Ask Spread:   " << getSpread(bestBid, bestAsk) << std::endl;
    }
};

#endif