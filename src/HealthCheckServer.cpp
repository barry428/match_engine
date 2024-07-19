#include "HealthCheckServer.h"
#include "Serialization.h"
#include <iostream>
#include <sstream>
#include <mutex>

HealthCheckServer::HealthCheckServer(const std::string& host, int port, zmq::context_t& context, const std::string& bookServerAddress)
        : host_(host), port_(port), bookSocket(context, zmq::socket_type::sub), running_(false) {
    bookSocket.connect(bookServerAddress);
    bookSocket.set(zmq::sockopt::subscribe, ""); // 订阅所有消息
}

void HealthCheckServer::start() {
    running_ = true;

    // 启动接收线程
    receiveThread_ = std::thread(&HealthCheckServer::receiveOrderBook, this);

    svr_.Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        LOG_DEBUG("Received GET request");

        std::ostringstream oss;
        oss << "<html><head><title>Health Check</title></head><body>";

        try {
            std::lock_guard<std::mutex> lock(orderBookMutex_);
            oss << "<pre>" << latestOrderBook_ << "</pre>"; // 输出最新的 orderBook

            oss << "</body></html>";

            res.set_content(oss.str(), "text/html");
            LOG_DEBUG("Response prepared: " + oss.str());
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in handle_request: " + std::string(e.what()));
            res.status = 500;
            res.set_content("Internal server error", "text/html");
        }
    });

    LOG_DEBUG("Starting server at " + host_ + ":" + std::to_string(port_));
    svr_.listen(host_.c_str(), port_);
}

void HealthCheckServer::stop() {
    running_ = false;
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    svr_.stop();
}

void HealthCheckServer::receiveOrderBook() {
    LOG_DEBUG("OrderBook receiver started.");
    while (running_) {
        try {
            zmq::message_t message;
            auto result = bookSocket.recv(message, zmq::recv_flags::none);
            if (result) {
                std::string orderBookData(static_cast<char*>(message.data()), message.size());
                LOG_DEBUG("Received order book data: " + orderBookData);

                std::lock_guard<std::mutex> lock(orderBookMutex_);
                latestOrderBook_ = orderBookData;
            }
        } catch (const zmq::error_t& e) {
            LOG_ERROR("ZeroMQ error in receiveOrderBook: " + std::string(e.what()));
        } catch (const std::exception& e) {
            LOG_ERROR("Error in receiveOrderBook: " + std::string(e.what()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_DEBUG("OrderBook receiver stopped.");
}
