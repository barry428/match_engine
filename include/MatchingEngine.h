#pragma once

#include <thread>
#include <map>
#include <cmath>
#include <zmq.hpp>
#include <unordered_map>
#include "Order.h"
#include "TradeRecord.h"
#include "Logger.h"

class MatchingEngine {
public:
    MatchingEngine(zmq::socket_t& orderSocket, zmq::socket_t& resultSocket, zmq::socket_t& bookSocket);
    void start();
    void stop();

private:
    void run();
    void processOrder(Order& order);
    void addOrderToBook(Order& order, std::map<double, std::map<unsigned int, Order>>& orderBook);
    void matchOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                     std::map<double, std::map<unsigned int, Order>>& ownOrders, bool isBuyOrder);
    void matchBuyOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                        std::map<double, std::map<unsigned int, Order>>& ownOrders);
    void matchSellOrders(Order& order, std::map<double, std::map<unsigned int, Order>>& oppositeOrders,
                         std::map<double, std::map<unsigned int, Order>>& ownOrders);
    void processTrade(Order& order, Order& oppositeOrder);
    void generateUnmatchedOrderMessage(const Order& order);
    void generateTradeMessage(const Order& buyOrder, const Order& sellOrder, const TradeRecord& trade);
    TradeRecord createTradeRecord(const Order& buyOrder, const Order& sellOrder, double tradeQuantity, double tradePrice, const std::string& orderType);
    double roundToPrecision(double value, int precision);
    void publishOrderBook();
    std::string formatOrderBook();

    zmq::socket_t& orderSocket;
    zmq::socket_t& resultSocket;
    zmq::socket_t& bookSocket;
    std::thread workerThread;
    bool running;

    std::map<double, std::map<unsigned int, Order>> buyOrders;
    std::map<double, std::map<unsigned int, Order>> sellOrders;

    // 用于控制订单簿发布频率的变量
    std::chrono::steady_clock::time_point lastPublishTime;
};

// 声明外部日志函数
extern void log(const std::string& message);

/*
 *
 * Demo
std::map<double, std::map<unsigned int, Order>> buyOrders;

// 创建一些订单
Order order1{10001, 1, 50000.0, 1.0, 0.0, 0.001, OrderSide::BUY, OrderType::LIMIT, OrderStatus::INITIAL, std::chrono::system_clock::now(), std::chrono::system_clock::now()};
Order order2{10002, 2, 51000.0, 2.0, 0.0, 0.001, OrderSide::BUY, OrderType::LIMIT, OrderStatus::INITIAL, std::chrono::system_clock::now(), std::chrono::system_clock::now()};
Order order3{10003, 3, 50000.0, 1.5, 0.0, 0.001, OrderSide::BUY, OrderType::LIMIT, OrderStatus::INITIAL, std::chrono::system_clock::now(), std::chrono::system_clock::now()};
Order order4{10004, 4, 52000.0, 0.5, 0.0, 0.001, OrderSide::BUY, OrderType::LIMIT, OrderStatus::INITIAL, std::chrono::system_clock::now(), std::chrono::system_clock::now()};

// 将订单添加到买单订单簿
buyOrders[50000.0][10001] = order1;
buyOrders[51000.0][10002] = order2;
buyOrders[50000.0][10003] = order3;
buyOrders[52000.0][10004] = order4;

*/

