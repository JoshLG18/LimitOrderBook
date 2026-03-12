#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <chrono>
#include <unordered_map>
#include "order.h"
#include "trade.h"
#include "analytics.h"

// Create a class to represent the order book
class OrderBook {
private:
    // create a map for buy and sell orders
    std::map<int64_t, std::queue<Order>> bids; // buy orders
    std::map<int64_t, std::queue<Order>> asks; // sell orders
    std::vector<Trade> trades; // stores the trades made
    std::unordered_map<int, std::pair<OrderSide, int64_t>> orderIndex; // maps order ID to its side and price for easy cancellation

    // function to match orders - match buy and sell orders based on price and quantity
    void matchOrders(std::chrono::high_resolution_clock::time_point start) {
        while (!bids.empty() && !asks.empty()){ // loop while both queues aren't empty
            auto highestBid = bids.rbegin(); // get the highest bid price
            auto lowestAsk = asks.begin(); // get the lowest sell price

            if (highestBid->first >= lowestAsk->first){ // check if the best buy price is greater or equal to the best sell price
                // if so we can make a trade

                Order& buyOrder = highestBid->second.front(); // get the first order for highestbid and lowest ask
                Order& sellOrder = lowestAsk->second.front();

                // make sure the quantity of the by is smaller than the sell order
                int64_t tradeQty = std::min(buyOrder.quantity, sellOrder.quantity); // store the trade quantity as the min of the by and sell order

                auto end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                trades.push_back(Trade{buyOrder.id, sellOrder.id, tradeQty, lowestAsk->first, latency, std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch())});
                analytics.recordTrade(trades.back());

                // update the quantities -> reduce amounts by the traded amount
                buyOrder.quantity -= tradeQty;
                sellOrder.quantity -= tradeQty;

                // increase the filled quantity for both orders
                buyOrder.filledQty += tradeQty;
                sellOrder.filledQty += tradeQty;

                // remove full filled orders
                if (buyOrder.quantity == 0) {
                    buyOrder.status = OrderStatus::FILLED; // update the order status to filled
                    orderIndex.erase(buyOrder.id); // remove the order from the index
                    highestBid->second.pop(); // remove the order from the queue
                } else {
                    buyOrder.status = OrderStatus::PARTIAL; // update the order status to partial if it's not fully filled
                }

                if (sellOrder.quantity == 0) {
                    sellOrder.status = OrderStatus::FILLED; // update the order status to filled
                    orderIndex.erase(sellOrder.id); // remove the order from the index
                    lowestAsk->second.pop();
                } else {
                    sellOrder.status = OrderStatus::PARTIAL; // update the order status to partial if it's not fully filled
                }

                // remove empty price levels
                if (highestBid->second.empty()) bids.erase(highestBid->first);
                if (lowestAsk->second.empty()) asks.erase(lowestAsk->first);

            } else {
                break; // no more matches possible
            }
            
        }
    }

    // function to execute market orders
    void executeMarketOrder(Order order, std::chrono::high_resolution_clock::time_point start) {
        if (order.side == OrderSide::BUY) {
            while (order.quantity > 0 && !asks.empty()) {
                auto lowestAsk = asks.begin();
                
                // get the first order at this price level
                Order& sellOrder = lowestAsk->second.front();
                
                // define the trade quantity
                int64_t tradeQty = std::min(order.quantity, sellOrder.quantity);

                auto end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                // store the match
                trades.push_back(Trade{order.id, sellOrder.id, tradeQty, lowestAsk->first, latency, std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch())});
                analytics.recordTrade(trades.back());

                // update quantities
                order.quantity -= tradeQty;
                sellOrder.quantity -= tradeQty;

                order.filledQty += tradeQty;
                sellOrder.filledQty += tradeQty;
                
                // remove filled orders and empty price levels
                if (sellOrder.quantity == 0) {
                    sellOrder.status = OrderStatus::FILLED; // update the order status to filled
                    orderIndex.erase(sellOrder.id); // remove the order from the index
                    lowestAsk->second.pop(); // remove the order from the queue
                } else {
                    sellOrder.status = OrderStatus::PARTIAL; // update the order status to partial if it's not fully filled
                }
                if (lowestAsk->second.empty()) asks.erase(lowestAsk->first);
            }
        } else {
            while (order.quantity > 0 && ! bids.empty()){
                auto highestBid = bids.rbegin();

                Order& buyOrder = highestBid->second.front();

                int64_t tradeQty = std::min(order.quantity, buyOrder.quantity);

                auto end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                // store the match
                trades.push_back(Trade{buyOrder.id, order.id,  tradeQty, highestBid->first, latency, std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch())});
                analytics.recordTrade(trades.back());

                order.quantity -= tradeQty;
                buyOrder.quantity -= tradeQty;

                order.filledQty += tradeQty;
                buyOrder.filledQty += tradeQty;

                if (buyOrder.quantity == 0) {
                    buyOrder.status = OrderStatus::FILLED; // update the order status to filled
                    orderIndex.erase(buyOrder.id); // remove the order from the index
                    highestBid->second.pop(); // remove the order from the queue
                } else {
                    buyOrder.status = OrderStatus::PARTIAL; // update the order status to partial if it's not fully filled
                }
                if (highestBid->second.empty()) {                
                    bids.erase(highestBid->first);
                }
            }
        }
    }

public:
    Analytics analytics;
    double getBestBid() const {
        return bids.empty() ? 0.0 : fromTicks(bids.rbegin()->first);
    }
    double getBestAsk() const {
        return asks.empty() ? 0.0 : fromTicks(asks.begin()->first);
    }

    // create a function to add an order to the book
    void addOrder(Order order){
        auto start = std::chrono::high_resolution_clock::now();
        analytics.recordOrder(order.side);
        if (order.type == OrderType::LIMIT) {
            if (order.side == OrderSide::BUY) {
                bids[order.price].push(order);
            } else {
                asks[order.price].push(order);
            }
            // add the order to the index for easy cancellation
            orderIndex[order.id] = {order.side, order.price};
            matchOrders(start);
        }  else {
            // Market order logic
            executeMarketOrder(order, start);
        }
    }

    // create a function to show the current state of the order book
    void showOrderBook() {
        std::cout << "\nCurrent Order Book:" << std::endl;
        std::cout << "-------------------------" << std::endl;

        std::cout << "\nBids:" << std::endl;
        std::cout << "-------------------------" << std::endl;
        for (auto it = bids.rbegin(); it != bids.rend(); ++it) {
            int64_t totalVolume = 0;
            std::queue<Order> temp = it->second; // copy the queue
            while (!temp.empty()) {
                totalVolume += temp.front().quantity;
                temp.pop();
            }
            std::cout << "Price: " << fromTicks(it->first) << " Quantity: " << totalVolume << std::endl;
        }
        std::cout << "-------------------------" << std::endl;

        std::cout << "\nAsks:" << std::endl;
        std::cout << "-------------------------" << std::endl;
        for (auto it = asks.begin(); it != asks.end(); ++it) {
            int64_t totalVolume = 0;
            std::queue<Order> temp = it->second; // copy the queue
            while (!temp.empty()) {
                totalVolume += temp.front().quantity;
                temp.pop();
            }
            std::cout << "Price: " << fromTicks(it->first) << " Quantity: " << totalVolume << std::endl;
        }
        std::cout << "-------------------------" << std::endl;

    }

    // function to cancel orders - remove an order from the book based on its ID
    bool cancelOrder(int orderId) {
        auto indexIt = orderIndex.find(orderId); // find the order in the index
        if (indexIt == orderIndex.end())  { // if not found, return false
            std::cout << "Order " << orderId << " not found\n";
            return false;
        }
        
        // get the side and price of the order from the index
        OrderSide side = indexIt->second.first;   
        int64_t price = indexIt->second.second;   

        // find the order in the corresponding queue and remove it
        auto& book = (side == OrderSide::BUY) ? bids : asks; // if the order is a buy order, we look in the bids, otherwise we look in the asks
        auto levelIt = book.find(price); // find the price level in the book

        // if the price level is found, we need to remove the order from the queue
        if (levelIt != book.end()) {
            std::queue<Order> temp; // create the temp queue
            while (!levelIt->second.empty()) { // loop through the orders at this price level
                Order o = levelIt->second.front(); // get the first order in the queue
                levelIt->second.pop(); // remove the order from the queue
                if (o.id == orderId) { // if this is the order we want to cancel, we skip adding it back to the queue
                    o.status = OrderStatus::CANCELLED; // update the order status to cancelled
                    std::cout << "Order " << orderId << " cancelled\n";
                } else {
                    temp.push(o); // if it's not the order we want to cancel, we add it back to the temp queue
                }
            }
            levelIt->second = temp; // replace the original queue with the temp queue that doesn't contain the cancelled order

            if (levelIt->second.empty()) {
                book.erase(levelIt); // if there are no more orders at this price level, we remove the price level from the book
            }
        }

        orderIndex.erase(indexIt); // remove the order from the index
        return true;
        }

    

    // function to show all the trades made
    void showTrades() {
        std::cout << "\nTrades made: " << std::endl;
        std::cout << "-------------------------" << std::endl;
        for (auto it = trades.begin(); it != trades.end(); ++it) {
            std::cout << "Buy ID: " << it->buyOrderID 
                << " Sell ID: " << it->sellOrderID
                << " Price: " << fromTicks(it->price)
                << " Quantity: " << it->quantity 
                << " Latency: " << it->latencyNs << std::endl;
        }
    }
};

#endif
