#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "TradeRecord.h"

enum class OrderSide {
    BUY,
    SELL,
    UNKNOWN
};

enum class OrderType {
    LIMIT,
    MARKET,
    UNKNOWN
};

enum class OrderStatus {
    INITIAL,
    MATCHING,
    PARTIALLY_FILLED,
    FULLY_FILLED,
    CANCELED,
    PARTIALLY_FILLED_CANCELED,
    UNKNOWN
};

struct Order {
    unsigned int orderId;
    unsigned long long userId;
    double price;
    double quantity;
    double feeRate;
    OrderSide orderSide;
    OrderType orderType;
    OrderStatus status;
    std::chrono::time_point<std::chrono::system_clock> createTime;
    std::chrono::time_point<std::chrono::system_clock> updateTime;
    double filledQuantity;

    // Comparison operators for priority_queue
    bool operator<(const Order& other) const {
        return price < other.price || (price == other.price && createTime > other.createTime);
    }

    bool operator>(const Order& other) const {
        return price > other.price || (price == other.price && createTime < other.createTime);
    }
};
