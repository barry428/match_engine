#pragma once

#include <random>
#include <atomic>
#include <zmq.hpp>
#include "Order.h"
#include "DbConnection.h"
#include "Logger.h"

class OrderGenerator {
public:

    OrderGenerator(DbConnection& dbConn, zmq::context_t& context, const std::string& orderServerAddress);
    void generateOrders(int numOrders);

private:
    Order createRandomOrder();
    OrderSide getRandomOrderSide();
    OrderType getRandomOrderType();
    void sendOrder(const Order& order, bool isUpdate = false);
    void writeOrderToDatabase(const Order& order);  // 新增方法
    void loadOrdersFromDatabase();
    unsigned int getMaxOrderId();
    double roundToPrecision(double value, int precision);

    zmq::socket_t orderSocket;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> priceDistribution;
    std::uniform_real_distribution<double> quantityDistribution;
    std::uniform_real_distribution<double> feeRateDistribution;
    std::uniform_int_distribution<int> orderSideDistribution;
    std::uniform_int_distribution<int> orderTypeDistribution;
    static std::atomic<unsigned int> orderIdCounter;

    DbConnection& dbConn;  // 新增数据库连接成员变量
};
