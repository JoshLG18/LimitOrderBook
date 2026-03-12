#ifndef ORDER_H
#define ORDER_H

#include <chrono>  
#include <cstdint> 

// create types
enum class OrderSide {BUY, SELL};
enum class OrderType {LIMIT, MARKET};
enum class OrderStatus {
    PENDING, 
    PARTIAL,
    FILLED,
    CANCELLED
};

static constexpr int64_t TICK_DIVISOR = 100; // constant which is the number to multiply the price by to get the correct rep

// function to convert between double and fixed-point -> human input into machine rep
inline int64_t toTicks(double price){
    return static_cast<int64_t>(price * TICK_DIVISOR + 0.5); // 0.5 makes sure the rounding is correct
}

// inline used to reduce overhead of the function call
inline double fromTicks(int64_t ticks){ // -> machine to human readable
    return static_cast<double>(ticks) / TICK_DIVISOR;
}

// Create a class to represent an order
class Order {
public:
    int id;
    OrderSide side;
    int64_t price;      // fixed-point ticks, not double
    int64_t quantity;   // remaining quantity
    int64_t filledQty;  // how much has been filled so far
    OrderType type;
    OrderStatus status;
    std::chrono::nanoseconds timestamp; // when the order is placed

    Order(int id, OrderSide side, int64_t price, int64_t quantity, OrderType type)
        : id(id),
          side(side),
          price(price),
          quantity(quantity),
          filledQty(0),
          type(type),
          status(OrderStatus::PENDING),
          // stores the nanoseconds from the reference period
          timestamp(std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())) {}
};

#endif
