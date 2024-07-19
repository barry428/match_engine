#pragma once

#include <string>
#include <chrono>


struct TradeRecord {
    unsigned int tradeId;
    unsigned long long buyerUserId;
    unsigned long long sellerUserId;
    unsigned int buyerOrderId;
    unsigned int sellerOrderId;
    std::string orderType;
    double tradePrice;
    double tradeQuantity;
    double buyerFee;
    double sellerFee;
    std::chrono::time_point<std::chrono::system_clock> tradeTime;
};