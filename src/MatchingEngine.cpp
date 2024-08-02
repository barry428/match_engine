#include "MatchingEngine.h"
#include "Serialization.h"
#include <iostream>
#include <string>
#include <zmq.hpp>

MatchingEngine::MatchingEngine(zmq::socket_t& orderSocket, zmq::socket_t& resultSocket, zmq::socket_t& bookSocket)
        : orderSocket(orderSocket), resultSocket(resultSocket), bookSocket(bookSocket), running(false) {}

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
                LOG_INFO("Order received: " + orderData);
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
    LOG_ERROR("processOrder executed in " + std::to_string(duration) + " μs.");
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
            addOrderToBook(order, ownOrders);
        }
    }

    // 发送 orderBook 消息
    publishOrderBook();

}

void MatchingEngine::publishOrderBook() {
    std::string orderBookData = formatOrderBook();
    zmq::message_t message(orderBookData.c_str(), orderBookData.size());
    bookSocket.send(message, zmq::send_flags::none);
    LOG_DEBUG("Published order book data: " + orderBookData);
}

void MatchingEngine::matchBuyOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                                    std::map<double, std::map<unsigned int, Order>>& ownOrders) {
    auto it = oppositeOrders.begin();
    while (it != oppositeOrders.end() && order.quantity > order.filledQuantity) {
        double bestPrice = it->first;
        LOG_DEBUG("Attempting to match buy order. BuyOrderID: " + std::to_string(order.orderId) +
                 ", BuyPrice: " + std::to_string(order.price) + ", BestSellPrice: " + std::to_string(bestPrice));
        if (order.price >= bestPrice) {
            auto& ordersAtPrice = it->second;
            for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && order.quantity > order.filledQuantity; ) {
                Order& oppositeOrder = orderIt->second;
                LOG_DEBUG("Processing trade. BuyOrderID: " + std::to_string(order.orderId) + ", SellOrderID: " + std::to_string(oppositeOrder.orderId));
                processTrade(order, oppositeOrder, "BUY");

                if (oppositeOrder.filledQuantity >= oppositeOrder.quantity) {
                    LOG_DEBUG("Sell order fully filled. SellOrderID: " + std::to_string(oppositeOrder.orderId));
                    orderIt = ordersAtPrice.erase(orderIt);
                } else {
                    // 更新订单状态
                    addOrderToBook(oppositeOrder, oppositeOrders);
                    ++orderIt;
                }
            }

            if (ordersAtPrice.empty()) {
                LOG_DEBUG("All sell orders at price " + std::to_string(bestPrice) + " have been matched.");
                it = oppositeOrders.erase(it);
            } else {
                ++it;
            }
        } else {
            break;
        }
    }
}

void MatchingEngine::matchSellOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                                     std::map<double, std::map<unsigned int, Order>>& ownOrders) {
    auto it = oppositeOrders.rbegin();
    while (it != oppositeOrders.rend() && order.quantity > order.filledQuantity) {
        double bestPrice = it->first;
        LOG_DEBUG("Attempting to match sell order. SellOrderID: " + std::to_string(order.orderId) +
                 ", SellPrice: " + std::to_string(order.price) + ", BestBuyPrice: " + std::to_string(bestPrice));
        if (order.price <= bestPrice) {
            auto& ordersAtPrice = it->second;
            for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && order.quantity > order.filledQuantity; ) {
                Order& oppositeOrder = orderIt->second;
                LOG_DEBUG("Processing trade. SellOrderID: " + std::to_string(order.orderId) + ", BuyOrderID: " + std::to_string(oppositeOrder.orderId));
                processTrade(order, oppositeOrder, "SELL");

                if (oppositeOrder.filledQuantity >= oppositeOrder.quantity) {
                    LOG_DEBUG("Buy order fully filled. BuyOrderID: " + std::to_string(oppositeOrder.orderId));
                    orderIt = ordersAtPrice.erase(orderIt);
                } else {
                    // 更新订单状态
                    addOrderToBook(oppositeOrder, oppositeOrders);
                    ++orderIt;
                }
            }

            if (ordersAtPrice.empty()) {
                LOG_DEBUG("All buy orders at price " + std::to_string(bestPrice) + " have been matched.");
                it = std::map<double, std::map<unsigned int, Order>>::reverse_iterator(oppositeOrders.erase(std::next(it).base()));
            } else {
                ++it;
            }
        } else {
            break;
        }
    }
}

void MatchingEngine::processTrade(Order& order, Order& oppositeOrder, const std::string& orderType) {
    LOG_DEBUG("Match start. OrderId: " + std::to_string(order.orderId) + " OrderPrice: " + std::to_string(order.price)
             + " OppositeOrderId: " + std::to_string(oppositeOrder.orderId) + " OppositeOrderPrice: " + std::to_string(oppositeOrder.price));

    if(oppositeOrder.filledQuantity >= oppositeOrder.quantity){
        return;
    }

    double tradeQuantity = std::min(order.quantity - order.filledQuantity, oppositeOrder.quantity - oppositeOrder.filledQuantity);
    double tradePrice = oppositeOrder.price;

    TradeRecord trade = createTradeRecord(order, oppositeOrder, tradeQuantity, tradePrice, orderType);

    double prevFilledQuantityOppositeOrder = oppositeOrder.filledQuantity;

    order.filledQuantity += tradeQuantity;
    oppositeOrder.filledQuantity += tradeQuantity;

    if (oppositeOrder.quantity > oppositeOrder.filledQuantity) {
        LOG_DEBUG("Update Order Status. PARTIALLY_FILLED OrderId: " + std::to_string(oppositeOrder.orderId) + " prevFilledQuantityOrder: " + std::to_string(prevFilledQuantityOppositeOrder) + " filledQuantity: " + std::to_string(oppositeOrder.filledQuantity));
    }else{
        LOG_DEBUG("Update Order Status. FULLY_FILLED OrderId : " + std::to_string(oppositeOrder.orderId) + " prevFilledQuantityOrder: " + std::to_string(prevFilledQuantityOppositeOrder) + " filledQuantity: " + std::to_string(oppositeOrder.filledQuantity));
    }

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
    trade.buyerFee = roundToPrecision(buyOrder.feeRate * tradeQuantity * tradePrice, 6);
    trade.sellerFee = roundToPrecision(sellOrder.feeRate * tradeQuantity * tradePrice, 6);
    trade.tradeTime = std::chrono::system_clock::now();
    return trade;
}

double MatchingEngine::roundToPrecision(double value, int precision) {
    double factor = std::pow(10.0, precision);
    return std::round(value * factor) / factor;
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
                << std::setw(8) << std::fixed << std::setprecision(2) << price << " | "
                << std::setw(8) << order.quantity << "\n";
        }
    }

    // 遍历 sellOrders
    for (const auto& [price, ordersAtPrice] : sellOrders) {
        for (const auto& [orderId, order] : ordersAtPrice) {
            oss << std::left << std::setw(12) << "SELL" << " | "
                << std::setw(8) << std::fixed << std::setprecision(2) << price << " | "
                << std::setw(8) << order.quantity << "\n";
        }
    }

    return oss.str();
}