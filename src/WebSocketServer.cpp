#include "WebSocketServer.h"
#include <iostream>

WebSocketServer::WebSocketServer(const std::string& host, int port, zmq::context_t& context, const std::string& bookServerAddress)
        : host_(host), port_(port), bookSocket(context, zmq::socket_type::sub), running_(false) {
    bookSocket.connect(bookServerAddress);
    bookSocket.set(zmq::sockopt::subscribe, ""); // 订阅所有消息
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    running_ = true;

    // 启动接收线程
    receiveThread_ = std::thread(&WebSocketServer::receive_order_book, this);

    m_server.init_asio();
    m_server.set_open_handler(bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    m_server.set_close_handler(bind(&WebSocketServer::on_close, this, std::placeholders::_1));
    m_server.set_message_handler(bind(&WebSocketServer::on_message, this, std::placeholders::_1, std::placeholders::_2));

    m_server.listen(port_);
    m_server.start_accept();
    m_server.run();
}

void WebSocketServer::stop() {
    running_ = false;
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    m_server.stop_listening();
}

void WebSocketServer::receive_order_book() {
    while (running_) {
        try {
            zmq::message_t message;
            auto result = bookSocket.recv(message, zmq::recv_flags::none);
            if (result) {
                std::string orderBookData(static_cast<char*>(message.data()), message.size());
                std::lock_guard<std::mutex> lock(orderBookMutex_);
                latestOrderBook_ = orderBookData;
                send_to_all(orderBookData);
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "ZeroMQ error in receive_order_book: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error in receive_order_book: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void WebSocketServer::send_to_all(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_connection_lock);
    for (auto conn : m_connections) {
        m_server.send(conn, message, websocketpp::frame::opcode::text);
    }
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_lock);
    m_connections.insert(hdl);
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_lock);
    m_connections.erase(hdl);
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;
}
