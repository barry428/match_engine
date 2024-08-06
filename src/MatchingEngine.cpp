#include "MatchingEngine.h"
#include "Serialization.h"
#include <iostream>
#include <string>
#include <zmq.hpp>

MatchingEngine::MatchingEngine(zmq::socket_t& orderSocket, zmq::socket_t& resultSocket, zmq::socket_t& bookSocket)
        : orderSocket(orderSocket), resultSocket(resultSocket), bookSocket(bookSocket), running(false) {
    std::chrono::steady_clock::time_point lastPublishTime;
}

void MatchingEngine::start() {
    LOG_INFO("MatchingEngine starting.");
    running = true;
    run();
    LOG_INFO("MatchingEngine started.");
}

void MatchingEngine::stop() {
    running = false;
}

void MatchingEngine::run() {
    LOG_INFO("MatchingEngine run.");
    while (running) {
        try {
            zmq::message_t orderMessage;
            auto result = orderSocket.recv(orderMessage, zmq::recv_flags::none);
            if (result.has_value()) {
                std::string orderData(static_cast<char*>(orderMessage.data()), orderMessage.size());
                LOG_DEBUG("Order received: " + orderData);
                if (!orderData.empty()) {
                    Json::Value message = deserializeMessage(orderData);
                    Json::Value nestedOrderMessage = deserializeMessage(message["order"].asString());
                    Order order = deserializeOrder(nestedOrderMessage);
                    processOrder(order);
                }
            } else {
                LOG_ERROR("No message received.");
            }
        } catch (const zmq::error_t& e) {
            LOG_ERROR("ZeroMQ error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            LOG_ERROR("Error in MatchingEngine run loop: " + std::string(e.what()));
        }
    }
}

void MatchingEngine::processOrder(Order& order) {
    auto start = std::chrono::high_resolution_clock::now();

    // 处理撮合订单（后续还可以处理订单取消的情况）
    if (order.orderSide == OrderSide::BUY) {
        matchOrders(order, sellOrders, buyOrders, true);
    } else {
        matchOrders(order, buyOrders, sellOrders, false);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    LOG_INFO("processOrder executed in " + std::to_string(duration) + " μs.");
}

void MatchingEngine::addOrderToBook(Order& order, std::map<double, std::map<unsigned int, Order>>& orderBook) {
    orderBook[order.price][order.orderId] = order;
}

void MatchingEngine::matchOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                                 std::map<double, std::map<unsigned int, Order>>& ownOrders, bool isBuyOrder) {
    if (isBuyOrder) {
        matchBuyOrders(order, oppositeOrders, ownOrders);
    } else {
        matchSellOrders(order, oppositeOrders, ownOrders);
    }

    // 记录主动担的所有状态变化
    if (order.filledQuantity == 0) {
        generateUnmatchedOrderMessage(order);
        LOG_DEBUG("matchOrders Update Order Status. MATCHING OrderId : " + std::to_string(order.orderId));
        addOrderToBook(order, ownOrders);
    }else{
        if (order.quantity > order.filledQuantity) {
            LOG_DEBUG("matchOrders Update Order Status. PARTIALLY_FILLED OrderId: " + std::to_string(order.orderId) + " filledQuantity: " + std::to_string(order.filledQuantity));
            addOrderToBook(order, ownOrders);
        }else{
            LOG_DEBUG("matchOrders Update Order Status. FULLY_FILLED OrderId : " + std::to_string(order.orderId));
        }
    }

    // 发送 orderBook 消息
    publishOrderBook();

}

void MatchingEngine::matchBuyOrders(Order& buyOrder, std::map<double, std::map<unsigned int, Order>>& sellOrders,
                                    std::map<double, std::map<unsigned int, Order>>& buyOrders) {
    // 遍历卖订单，优先匹配最低的卖价
    for (auto it = sellOrders.begin(); it != sellOrders.end() && buyOrder.quantity > buyOrder.filledQuantity; ) {
        double sellPrice = it->first;

        // 如果买单价格小于卖单价格，停止匹配
        if (buyOrder.price < sellPrice) {
            break;
        }

        auto& ordersAtPrice = it->second;
        for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && buyOrder.quantity > buyOrder.filledQuantity; ) {
            Order& sellOrder = orderIt->second;

            // 进行交易处理
            processTrade(buyOrder, sellOrder);

            // 如果卖单已完全成交，移除该卖单
            if (sellOrder.filledQuantity >= sellOrder.quantity) {
                orderIt = ordersAtPrice.erase(orderIt);
            } else {
                // 卖单部分成交，更新后继续
                ++orderIt;
            }
        }

        // 如果该价格下的所有卖单都已处理完，移除该价格节点
        if (ordersAtPrice.empty()) {
            it = sellOrders.erase(it);
        } else {
            ++it;
        }
    }
}

void MatchingEngine::matchSellOrders(Order& sellOrder, std::map<double, std::map<unsigned int, Order>>& buyOrders,
                                     std::map<double, std::map<unsigned int, Order>>& sellOrders) {
    // 使用反向迭代器，从最高买价开始匹配
    for (auto it = buyOrders.rbegin(); it != buyOrders.rend() && sellOrder.quantity > sellOrder.filledQuantity; ) {
        double buyPrice = it->first;

        // 如果卖单价格高于买单价格，停止匹配
        if (sellOrder.price > buyPrice) {
            break;
        }

        auto& ordersAtPrice = it->second;
        for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && sellOrder.quantity > sellOrder.filledQuantity; ) {
            Order& buyOrder = orderIt->second;

            // 执行交易
            processTrade(sellOrder, buyOrder);

            // 如果买单已完全成交，移除该买单
            if (buyOrder.filledQuantity >= buyOrder.quantity) {
                orderIt = ordersAtPrice.erase(orderIt);
            } else {
                // 买单部分成交，更新后继续
                ++orderIt;
            }
        }

        // 如果该价格下的所有买单都已处理完，移除该价格节点
        if (ordersAtPrice.empty()) {
            it = std::map<double, std::map<unsigned int, Order>>::reverse_iterator(buyOrders.erase(std::next(it).base()));
        } else {
            ++it;
        }
    }
}


void MatchingEngine::processTrade(Order& order, Order& oppositeOrder) {

    double tradeQuantity = std::min(order.quantity - order.filledQuantity, oppositeOrder.quantity - oppositeOrder.filledQuantity);
    double tradePrice = oppositeOrder.price;

    order.filledQuantity += tradeQuantity;
    oppositeOrder.filledQuantity += tradeQuantity;
    TradeRecord trade = createTradeRecord(order, oppositeOrder, tradeQuantity, tradePrice, orderSideToString(order.orderSide));

    generateTradeMessage(order, oppositeOrder, trade);

}

void MatchingEngine::generateUnmatchedOrderMessage(const Order& order) {
    Json::Value message;
    message["type"] = "UNMATCHED_ORDER";
    message["order"] = serializeOrder(order);

    std::string serializedMessage = serializeMessage(message);
    LOG_DEBUG("generateUnmatchedOrderMessage push: " + serializedMessage);
    zmq::message_t resultMessage(serializedMessage.data(), serializedMessage.size());
    resultSocket.send(resultMessage, zmq::send_flags::none);
}

void MatchingEngine::generateTradeMessage(const Order& buyOrder, const Order& sellOrder, const TradeRecord& trade) {
    Json::Value message;
    message["type"] = "TRADE";
    message["buyOrder"] = serializeOrder(buyOrder);
    message["sellOrder"] = serializeOrder(sellOrder);
    message["tradeRecord"] = serializeTradeRecord(trade);

    std::string serializedMessage = serializeMessage(message);
    LOG_DEBUG("generateTradeMessage push: " + serializedMessage);
    zmq::message_t resultMessage(serializedMessage.data(), serializedMessage.size());
    resultSocket.send(resultMessage, zmq::send_flags::none);
}

TradeRecord MatchingEngine::createTradeRecord(const Order& buyOrder, const Order& sellOrder, double tradeQuantity, double tradePrice, const std::string& orderType) {
    TradeRecord trade;
    trade.tradeId = rand();  // 可以改为更好的生成方法
    trade.buyerUserId = buyOrder.userId;
    trade.sellerUserId = sellOrder.userId;
    trade.buyerOrderId = buyOrder.orderId;
    trade.sellerOrderId = sellOrder.orderId;
    trade.orderType = orderType;
    trade.tradePrice = tradePrice;
    trade.tradeQuantity = tradeQuantity;
    trade.buyerFee = roundToPrecision(buyOrder.feeRate * tradeQuantity * tradePrice, 8);
    trade.sellerFee = roundToPrecision(sellOrder.feeRate * tradeQuantity * tradePrice, 8);
    trade.tradeTime = std::chrono::system_clock::now();
    return trade;
}

double MatchingEngine::roundToPrecision(double value, int precision) {
    double factor = std::pow(10.0, precision);
    return std::round(value * factor) / factor;
}

void MatchingEngine::publishOrderBook() {
    auto now = std::chrono::steady_clock::now();
    // 每秒最多发布一次订单簿
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPublishTime).count() >= 1) {
        lastPublishTime = now;
        std::string orderBookData = formatOrderBook();
        zmq::message_t message(orderBookData.c_str(), orderBookData.size());
        bookSocket.send(message, zmq::send_flags::none);
        LOG_DEBUG("Published order book");
    }
}

std::string MatchingEngine::formatOrderBook() {
    std::ostringstream oss;
    oss << std::left << std::setw(12) << "Order Side" << " | "
        << std::setw(8) << "Price" << " | "
        << std::setw(8) << "Quantity" << "\n";
    oss << std::string(36, '-') << "\n";

    // 遍历 buyOrders
    for (const auto& [price, ordersAtPrice] : buyOrders) {
        for (const auto& [orderId, order] : ordersAtPrice) {
            oss << std::left << std::setw(12) << "BUY" << " | "
                << std::setw(8) << std::fixed << std::setprecision(8) << price << " | "
                << std::setw(8) << order.quantity << "\n";
        }
    }

    // 遍历 sellOrders
    for (const auto& [price, ordersAtPrice] : sellOrders) {
        for (const auto& [orderId, order] : ordersAtPrice) {
            oss << std::left << std::setw(12) << "SELL" << " | "
                << std::setw(8) << std::fixed << std::setprecision(8) << price << " | "
                << std::setw(8) << order.quantity << "\n";
        }
    }

    return oss.str();
}