#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <zmq.hpp>
#include <set>
#include <mutex>
#include <thread>
#include <string>

typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketServer {
public:
    WebSocketServer(const std::string& host, int port, zmq::context_t& context, const std::string& bookServerAddress);
    ~WebSocketServer();
    void start();
    void stop();
    void send_to_all(const std::string& message);

private:
    void receive_order_book();
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

    server m_server;
    zmq::socket_t bookSocket;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_connections;
    std::mutex m_connection_lock;
    std::mutex orderBookMutex_;
    std::thread receiveThread_;
    bool running_;
    std::string latestOrderBook_;
    std::string host_;
    int port_;
};