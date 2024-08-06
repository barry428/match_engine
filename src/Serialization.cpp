#include "Serialization.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string time_point_to_string(const std::chrono::time_point<std::chrono::system_clock>& tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time));
    return std::string(buffer);
}

std::chrono::time_point<std::chrono::system_clock> string_to_time_point(const std::string& s) {
    std::tm tm = {};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string orderSideToString(OrderSide side) {
    switch (side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

OrderSide stringToOrderSide(const std::string& str) {
    if (str == "BUY") return OrderSide::BUY;
    if (str == "SELL") return OrderSide::SELL;
    return OrderSide::UNKNOWN;
}

std::string orderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::MARKET: return "MARKET";
        default: return "UNKNOWN";
    }
}

OrderType stringToOrderType(const std::string& str) {
    if (str == "LIMIT") return OrderType::LIMIT;
    if (str == "MARKET") return OrderType::MARKET;
    return OrderType::UNKNOWN;
}

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::INITIAL: return "INITIAL";
        case OrderStatus::MATCHING: return "MATCHING";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FULLY_FILLED: return "FULLY_FILLED";
        case OrderStatus::CANCELED: return "CANCELED";
        case OrderStatus::PARTIALLY_FILLED_CANCELED: return "PARTIALLY_FILLED_CANCELED";
        default: return "UNKNOWN";
    }
}

OrderStatus stringToOrderStatus(const std::string& str) {
    if (str == "INITIAL") return OrderStatus::INITIAL;
    if (str == "MATCHING") return OrderStatus::MATCHING;
    if (str == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
    if (str == "FULLY_FILLED") return OrderStatus::FULLY_FILLED;
    if (str == "CANCELED") return OrderStatus::CANCELED;
    if (str == "PARTIALLY_FILLED_CANCELED") return OrderStatus::PARTIALLY_FILLED_CANCELED;
    return OrderStatus::UNKNOWN;
}

std::string serializeOrder(const Order& order) {
    Json::Value root;
    root["orderId"] = order.orderId;
    root["userId"] = order.userId;
    root["price"] = std::to_string(order.price);
    root["quantity"] = std::to_string(order.quantity);
    root["feeRate"] = std::to_string(order.feeRate);
    root["orderSide"] = orderSideToString(order.orderSide);
    root["orderType"] = orderTypeToString(order.orderType);
    root["status"] = orderStatusToString(order.status);
    root["createTime"] = time_point_to_string(order.createTime);
    root["updateTime"] = time_point_to_string(order.updateTime);
    root["filledQuantity"] = std::to_string(order.filledQuantity);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 去掉换行符和缩进
    LOG_DEBUG("serialize orders. orderId:" + std::to_string(order.orderId) + " price:" + std::to_string(order.price));
    LOG_DEBUG("serialize orders. json:" + Json::writeString(writer, root));
    return Json::writeString(writer, root);
}

// 反序列化 Order 对象
Order deserializeOrder(const Json::Value& root) {
    Order order;
    if (!root.isObject()) {
        throw std::runtime_error("deserializeOrder: JSON root is not an object");
    }

    try {
        order.orderId = root["orderId"].asUInt();
        order.userId = root["userId"].asUInt64();
        order.price = convertStringToDouble(root, "price");
        order.quantity = convertStringToDouble(root, "quantity");
        order.feeRate = convertStringToDouble(root, "feeRate");
        order.orderSide = stringToOrderSide(root["orderSide"].asString());
        order.orderType = stringToOrderType(root["orderType"].asString());
        order.status = stringToOrderStatus(root["status"].asString());
        order.createTime = string_to_time_point(root["createTime"].asString());
        order.updateTime = string_to_time_point(root["updateTime"].asString());
        order.filledQuantity = convertStringToDouble(root, "filledQuantity");
    } catch (const std::exception& e) {
        throw std::runtime_error("Error deserializing Order: " + std::string(e.what()));
    }
    return order;
}


std::string serializeTradeRecord(const TradeRecord& trade) {
    Json::Value root;
    root["tradeId"] = trade.tradeId;
    root["buyerUserId"] = trade.buyerUserId;
    root["sellerUserId"] = trade.sellerUserId;
    root["buyerOrderId"] = trade.buyerOrderId;
    root["sellerOrderId"] = trade.sellerOrderId;
    root["orderType"] = trade.orderType;
    root["tradePrice"] = std::to_string(trade.tradePrice);
    root["tradeQuantity"] = std::to_string(trade.tradeQuantity);
    root["buyerFee"] = std::to_string(trade.buyerFee);
    root["sellerFee"] = std::to_string(trade.sellerFee);
    root["tradeTime"] = time_point_to_string(trade.tradeTime);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 去掉换行符和缩进
    return Json::writeString(writer, root);
}

// 反序列化 TradeRecord 对象
TradeRecord deserializeTradeRecord(const Json::Value& root) {
    TradeRecord trade;
    if (!root.isObject()) {
        throw std::runtime_error("deserializeTradeRecord: JSON root is not an object");
    }

    try {
        trade.tradeId = root["tradeId"].asUInt();
        trade.buyerUserId = root["buyerUserId"].asUInt64();
        trade.sellerUserId = root["sellerUserId"].asUInt64();
        trade.buyerOrderId = root["buyerOrderId"].asUInt();
        trade.sellerOrderId = root["sellerOrderId"].asUInt();
        trade.orderType = root["orderType"].asString();
        trade.tradePrice = convertStringToDouble(root, "tradePrice");
        trade.tradeQuantity = convertStringToDouble(root, "tradeQuantity");
        trade.buyerFee = convertStringToDouble(root, "buyerFee");
        trade.sellerFee = convertStringToDouble(root, "sellerFee");
        trade.tradeTime = string_to_time_point(root["tradeTime"].asString());
    } catch (const std::exception& e) {
        throw std::runtime_error("Error deserializing TradeRecord: " + std::string(e.what()));
    }
    return trade;
}

std::string serializeMessage(const Json::Value& message) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 去掉换行符和缩进
    return Json::writeString(writer, message);
}

Json::Value deserializeMessage(const std::string& data) {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::istringstream s(data);
    std::string errs;

    if (!Json::parseFromStream(reader, s, &root, &errs)) {
        LOG_DEBUG("Failed to parse JSON: " + errs);
        throw std::runtime_error("Failed to parse JSON");
    }

    return root;
}

// 转换字符串到 double 的辅助函数
double convertStringToDouble(const Json::Value& value, const std::string& key) {
    if (!value.isMember(key) || !value[key].isString()) {
        throw std::invalid_argument("Key is missing or not a string in JSON object: " + key);
    }

    const std::string& strValue = value[key].asString();
    try {
        return std::stod(strValue);
    } catch (const std::exception& e) {
        throw std::runtime_error("Conversion error for key " + key + ": " + std::string(e.what()));
    }
}