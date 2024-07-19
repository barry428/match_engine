#include "Serialization.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cmath>

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
    root["price"] = order.price;
    root["quantity"] = order.quantity;
    root["feeRate"] = order.feeRate;
    root["orderSide"] = orderSideToString(order.orderSide);
    root["orderType"] = orderTypeToString(order.orderType);
    root["status"] = orderStatusToString(order.status);
    root["createTime"] = time_point_to_string(order.createTime);
    root["updateTime"] = time_point_to_string(order.updateTime);
    root["filledQuantity"] = order.filledQuantity;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 去掉换行符和缩进
    return Json::writeString(writer, root);
}

Order deserializeOrder(const Json::Value& root) {
    Order order;
    try {
        if (!root.isObject()) {
            LOG_DEBUG("deserializeOrder: JSON root is not an object");
            throw std::runtime_error("JSON root is not an object");
        }

        if (!root.isMember("orderId")) {
            LOG_DEBUG("deserializeOrder: Missing 'orderId'");
            throw std::runtime_error("Missing 'orderId'");
        }
        order.orderId = root["orderId"].asUInt();

        if (!root.isMember("userId")) {
            LOG_DEBUG("deserializeOrder: Missing 'userId'");
            throw std::runtime_error("Missing 'userId'");
        }
        order.userId = root["userId"].asUInt64();

        if (!root.isMember("price")) {
            LOG_DEBUG("deserializeOrder: Missing 'price'");
            throw std::runtime_error("Missing 'price'");
        }
        order.price = roundToPrecision(root["price"].asDouble(), 8);

        if (!root.isMember("quantity")) {
            LOG_DEBUG("deserializeOrder: Missing 'quantity'");
            throw std::runtime_error("Missing 'quantity'");
        }
        order.quantity = roundToPrecision(root["quantity"].asDouble(), 6);

        if (!root.isMember("feeRate")) {
            LOG_DEBUG("deserializeOrder: Missing 'feeRate'");
            throw std::runtime_error("Missing 'feeRate'");
        }
        order.feeRate = roundToPrecision(root["feeRate"].asDouble(), 6);

        if (!root.isMember("orderSide")) {
            LOG_DEBUG("deserializeOrder: Missing 'orderSide'");
            throw std::runtime_error("Missing 'orderSide'");
        }
        order.orderSide = stringToOrderSide(root["orderSide"].asString());

        if (!root.isMember("orderType")) {
            LOG_DEBUG("deserializeOrder: Missing 'orderType'");
            throw std::runtime_error("Missing 'orderType'");
        }
        order.orderType = stringToOrderType(root["orderType"].asString());

        if (!root.isMember("status")) {
            LOG_DEBUG("deserializeOrder: Missing 'status'");
            throw std::runtime_error("Missing 'status'");
        }
        order.status = stringToOrderStatus(root["status"].asString());

        if (!root.isMember("createTime")) {
            LOG_DEBUG("deserializeOrder: Missing 'createTime'");
            throw std::runtime_error("Missing 'createTime'");
        }
        order.createTime = string_to_time_point(root["createTime"].asString());

        if (!root.isMember("updateTime")) {
            LOG_DEBUG("deserializeOrder: Missing 'updateTime'");
            throw std::runtime_error("Missing 'updateTime'");
        }
        order.updateTime = string_to_time_point(root["updateTime"].asString());

        if (!root.isMember("filledQuantity")) {
            LOG_DEBUG("deserializeOrder: Missing 'filledQuantity'");
            throw std::runtime_error("Missing 'filledQuantity'");
        }
        order.filledQuantity = roundToPrecision(root["filledQuantity"].asDouble(), 6);

    } catch (const std::exception& e) {
        LOG_DEBUG("Error deserializing Order: " + std::string(e.what()));
        throw;
    }
    return order;
}

TradeRecord deserializeTradeRecord(const Json::Value& root) {
    TradeRecord trade;
    try {
        if (!root.isObject()) {
            LOG_DEBUG("deserializeTradeRecord: JSON root is not an object");
            throw std::runtime_error("JSON root is not an object");
        }

        if (!root.isMember("tradeId")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'tradeId'");
            throw std::runtime_error("Missing 'tradeId'");
        }
        trade.tradeId = root["tradeId"].asUInt();

        if (!root.isMember("buyerUserId")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'buyerUserId'");
            throw std::runtime_error("Missing 'buyerUserId'");
        }
        trade.buyerUserId = root["buyerUserId"].asUInt64();

        if (!root.isMember("sellerUserId")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'sellerUserId'");
            throw std::runtime_error("Missing 'sellerUserId'");
        }
        trade.sellerUserId = root["sellerUserId"].asUInt64();

        if (!root.isMember("buyerOrderId")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'buyerOrderId'");
            throw std::runtime_error("Missing 'buyerOrderId'");
        }
        trade.buyerOrderId = root["buyerOrderId"].asUInt();

        if (!root.isMember("sellerOrderId")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'sellerOrderId'");
            throw std::runtime_error("Missing 'sellerOrderId'");
        }
        trade.sellerOrderId = root["sellerOrderId"].asUInt();

        if (!root.isMember("orderType")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'orderType'");
            throw std::runtime_error("Missing 'orderType'");
        }
        trade.orderType = root["orderType"].asString();

        if (!root.isMember("tradePrice")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'tradePrice'");
            throw std::runtime_error("Missing 'tradePrice'");
        }
        trade.tradePrice = roundToPrecision(root["tradePrice"].asDouble(), 8);

        if (!root.isMember("tradeQuantity")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'tradeQuantity'");
            throw std::runtime_error("Missing 'tradeQuantity'");
        }
        trade.tradeQuantity = roundToPrecision(root["tradeQuantity"].asDouble(), 6);

        if (!root.isMember("buyerFee")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'buyerFee'");
            throw std::runtime_error("Missing 'buyerFee'");
        }
        trade.buyerFee = roundToPrecision(root["buyerFee"].asDouble(), 6);

        if (!root.isMember("sellerFee")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'sellerFee'");
            throw std::runtime_error("Missing 'sellerFee'");
        }
        trade.buyerFee = roundToPrecision(root["sellerFee"].asDouble(), 6);

        if (!root.isMember("tradeTime")) {
            LOG_DEBUG("deserializeTradeRecord: Missing 'tradeTime'");
            throw std::runtime_error("Missing 'tradeTime'");
        }
        trade.tradeTime = string_to_time_point(root["tradeTime"].asString());

    } catch (const std::exception& e) {
        LOG_DEBUG("Error deserializing TradeRecord: " + std::string(e.what()));
        throw;
    }
    return trade;
}

std::string serializeTradeRecord(const TradeRecord& trade) {
    Json::Value root;
    root["tradeId"] = trade.tradeId;
    root["buyerUserId"] = trade.buyerUserId;
    root["sellerUserId"] = trade.sellerUserId;
    root["buyerOrderId"] = trade.buyerOrderId;
    root["sellerOrderId"] = trade.sellerOrderId;
    root["orderType"] = trade.orderType;
    root["tradePrice"] = trade.tradePrice;
    root["tradeQuantity"] = trade.tradeQuantity;
    root["buyerFee"] = trade.buyerFee;
    root["sellerFee"] = trade.sellerFee;
    root["tradeTime"] = time_point_to_string(trade.tradeTime);
    Json::StreamWriterBuilder writer;
    writer["indentation"] = ""; // 去掉换行符和缩进
    return Json::writeString(writer, root);
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

double roundToPrecision(double value, int precision) {
    double factor = std::pow(10.0, precision);
    return std::round(value * factor) / factor;
}