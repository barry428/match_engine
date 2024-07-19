#pragma once

#include <string>
#include <zmq.hpp>
#include <thread>
#include <mutex>
#include "httplib.h"
#include "Order.h"
#include "TradeRecord.h"
#include "Logger.h"

class HealthCheckServer {
public:
    HealthCheckServer(const std::string& host, int port, zmq::context_t& context, const std::string& bookServerAddress);

    void start();
    void stop();

private:
    void receiveOrderBook();

    httplib::Server svr_;
    std::string host_;
    zmq::socket_t bookSocket;
    int port_;
    bool running_;

    std::thread receiveThread_;
    std::string latestOrderBook_;
    std::mutex orderBookMutex_; // 保护 latestOrderBook_ 的互斥锁
};
