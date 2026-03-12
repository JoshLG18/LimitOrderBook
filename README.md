# Limit Order Book (C++)

A high-performance limit order book implementation in C++17, built to replicate core matching engine behaviour found in electronic exchanges and systematic trading infrastructure.

---

## Overview

An order book is the fundamental data structure of any exchange or trading venue. It maintains a record of all outstanding buy (bid) and sell (ask) orders, matches them when prices cross, and produces a trade record. This implementation prioritises correctness and performance, using design decisions typical of production trading systems.

---

## Features

- Limit and market order support
- Price-time priority matching (FIFO within each price level)
- O(1) order cancellation via hash-indexed order lookup
- Fixed-point integer price representation (eliminates floating-point map key bugs)
- Order status tracking: `PENDING`, `PARTIAL`, `FILLED`, `CANCELLED`
- Nanosecond-resolution timestamps on orders and trades
- Analytics: VWAP, average match latency, bid-ask spread, volume
- 7 unit tests via Catch2

---

## Design Decisions

### Fixed-point integer prices

Prices are stored as `int64_t` ticks rather than `double`. Floating-point values are unsuitable as `std::map` keys because rounding errors mean two logically equal prices may compare as unequal. A `TICK_DIVISOR` of 100 gives two decimal places of precision (e.g. £100.50 → 10050 ticks).

```cpp
static constexpr int64_t TICK_DIVISOR = 100;

inline int64_t toTicks(double price) {
    return static_cast<int64_t>(price * TICK_DIVISOR + 0.5);
}

inline double fromTicks(int64_t ticks) {
    return static_cast<double>(ticks) / TICK_DIVISOR;
}
```

### O(1) order cancellation

Naive cancellation requires scanning every price level and queue — O(n) in the number of orders. This implementation maintains an `unordered_map` from order ID to `{side, price}`, allowing direct lookup of which price level contains the order before performing the queue scan at that level only.

```
cancelOrder(id)  →  orderIndex lookup: O(1)
                 →  price level queue scan: O(k) where k = orders at that price
```

For typical order books where many price levels each have few orders, this is effectively O(1).

### Bid and ask storage

| Structure | Container                                           | Rationale                             |
| --------- | --------------------------------------------------- | ------------------------------------- |
| Bids      | `std::map<int64_t, std::queue<Order>>` (descending) | Highest bid at `rbegin()` in O(log n) |
| Asks      | `std::map<int64_t, std::queue<Order>>` (ascending)  | Lowest ask at `begin()` in O(log n)   |

Each price level holds a `std::queue` of orders, preserving time priority (FIFO) within a level.

### Complexity summary

| Operation           | Complexity                                   |
| ------------------- | -------------------------------------------- |
| Add limit order     | O(log n)                                     |
| Add market order    | O(k log n) where k = price levels consumed   |
| Cancel order        | O(1) lookup + O(m) queue scan at price level |
| Match orders        | O(log n) per match                           |
| Best bid / best ask | O(1)                                         |

---

## Project Structure

```
LimitOrderBook/
├── src/
│   └── main.cpp          # Interactive CLI
├── include/
│   ├── order.h           # Order struct, fixed-point helpers, OrderStatus
│   ├── trade.h           # Trade struct
│   ├── orderbook.h       # Core matching engine
│   └── analytics.h       # VWAP, latency, spread tracking
├── tests/
│   └── tests.cpp         # Catch2 unit tests
├── CMakeLists.txt
└── README.md
```

---

## Build Instructions

### Prerequisites

- CMake 3.15+
- Clang++ or G++ with C++17 support
- [vcpkg](https://github.com/microsoft/vcpkg) with Catch2 installed

```bash
vcpkg install catch2
```

### Build

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Run

```bash
./build/main      # interactive CLI
./build/tests     # unit tests
```

---

## Usage

The interactive CLI accepts the following actions:

```
1. Add order       — specify ID, side (BUY/SELL), type (LIMIT/MARKET), price, quantity
2. Cancel order    — cancel a resting limit order by ID
3. Show book       — display current bids and asks with aggregate volume per level
4. Show trades     — display all matched trades with price, quantity, and latency
5. Show analytics  — display VWAP, average match latency, spread, and order counts
6. Quit
```

### Example sequence

```
Add LIMIT BUY  id=1 price=100.50 qty=10   → rests on bid
Add LIMIT SELL id=2 price=101.00 qty=5    → rests on ask
Add LIMIT SELL id=3 price=100.50 qty=10   → crosses bid, triggers match
Show trades                                → Trade at 100.50, qty=10, latency=~400ns
```

---

## Tests

```bash
./build/tests
```

Current test coverage:

| Test                           | Description                           |
| ------------------------------ | ------------------------------------- |
| Full match at same price       | Buy and sell fully consume each other |
| No match when spread exists    | Orders rest without crossing          |
| Partial fill                   | Residual quantity remains on book     |
| Cancel removes order           | Cancelled order no longer appears     |
| Cancel non-existent order      | Returns false, no crash               |
| Market buy against resting ask | Fully consumes liquidity              |
| Market order with no liquidity | Handles empty book gracefully         |

---

## Roadmap

- [ ] Input validation (reject negative prices, zero quantities, duplicate IDs)
- [ ] File-based order feed (CSV replay mode)
- [ ] Latency benchmark (orders/second throughput measurement)
- [ ] Order book snapshot at point in time
