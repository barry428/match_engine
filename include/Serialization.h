#pragma once

#include <string>
#include <chrono>
#include "Order.h"
#include "TradeRecord.h"
#include "Logger.h"
#include <json/json.h>

// Helper functions for time serialization and deserialization
std::string time_point_to_string(const std::chrono::time_point<std::chrono::system_clock>& tp);
std::chrono::time_point<std::chrono::system_clock> string_to_time_point(const std::string& s);

// Enum to string conversion
std::string orderSideToString(OrderSide side);
OrderSide stringToOrderSide(const std::string& str);

std::string orderTypeToString(OrderType type);
OrderType stringToOrderType(const std::string& str);

std::string orderStatusToString(OrderStatus status);
OrderStatus stringToOrderStatus(const std::string& str);

// 序列化 Order 对象
std::string serializeOrder(const Order& order);
Order deserializeOrder(const Json::Value& root);

// 序列化 TradeRecord 对象
std::string serializeTradeRecord(const TradeRecord& trade);
TradeRecord deserializeTradeRecord(const Json::Value& root);

// 消息序列化
std::string serializeMessage(const Json::Value& message);
Json::Value deserializeMessage(const std::string& data);

double roundToPrecision(double value, int precision);
