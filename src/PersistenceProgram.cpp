#include "PersistenceProgram.h"
#include <iostream>

PersistenceProgram::PersistenceProgram(DbConnectionPool& connectionPool, zmq::context_t& context, const std::string& resultServerAddress)
        : dbConnPool(connectionPool), context(context), resultServerAddress(resultServerAddress), resultSocket(context, zmq::socket_type::pull), running(false) {

    try {
        dbConn = dbConnPool.getConnection(); // 获取连接
        if (dbConn == nullptr || dbConn->getConnection() == nullptr) {
            LOG_ERROR("Failed to get database connection.");
            throw std::runtime_error("Failed to get database connection");
        }

        resultSocket.connect(resultServerAddress);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while initializing PersistenceProgram: " + std::string(e.what()));
        throw; // Re-throw the exception after logging
    }
}

PersistenceProgram::~PersistenceProgram() {
    if (dbConn) {
        dbConnPool.returnConnection(dbConn); // 归还连接
    }
}

void PersistenceProgram::start() {
    LOG_INFO("PersistenceProgram starting.");
    running = true;
    workerThread = std::thread(&PersistenceProgram::run, this);
    LOG_INFO("PersistenceProgram started.");
}

void PersistenceProgram::stop() {
    LOG_INFO("PersistenceProgram stopping.");
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
    LOG_INFO("PersistenceProgram stopped.");
}

void PersistenceProgram::run() {
    LOG_INFO("PersistenceProgram running.");
    while (running) {
        try {
            zmq::message_t resultMessage;
            auto result = resultSocket.recv(resultMessage, zmq::recv_flags::none);
            if (!result) {
                LOG_ERROR("No message received.");
                continue;
            }

            std::string resultData(static_cast<char*>(resultMessage.data()), resultMessage.size());
            LOG_DEBUG("Received message from resultSocket: " + resultData);

            if (resultData.empty()) {
                LOG_ERROR("Received empty message.");
                continue;
            }

            Json::Value message;
            Json::CharReaderBuilder reader;
            std::string errs;
            std::istringstream s(resultData);

            if (!Json::parseFromStream(reader, s, &message, &errs)) {
                LOG_ERROR("Failed to parse message: " + errs);
                continue;
            }

            PersistenceProgram::processMessage(message);

        } catch (const zmq::error_t& e) {
            LOG_ERROR("ZeroMQ error in PersistenceProgram run loop: " + std::string(e.what()));
            reconnectResultClient();
        } catch (const std::exception& e) {
            LOG_ERROR("Error in PersistenceProgram run loop: " + std::string(e.what()));
            reconnectResultClient();
        }

    }
    LOG_INFO("PersistenceProgram stopped.");
}

void PersistenceProgram::processMessage(const Json::Value& message) {
    std::string messageType = message["type"].asString();
    if (messageType == "TRADE") {
        LOG_DEBUG("Processing TRADE message.");
        processTradeMessage(message);
    } else if (messageType == "UNMATCHED_ORDER") {
        LOG_DEBUG("Processing UNMATCHED_ORDER message.");
        processUnmatchedOrderMessage(message);
    } else {
        LOG_ERROR("Unknown message type received: " + messageType);
    }
}


void PersistenceProgram::reconnectResultClient() {
    while (running) {
        try {
            resultSocket.close();
            resultSocket = zmq::socket_t(context, zmq::socket_type::pull);
            resultSocket.connect(resultServerAddress);
            LOG_DEBUG("Reconnected to result server.");
            return;
        } catch (const zmq::error_t& e) {
            LOG_ERROR("Error reconnecting to result server: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
        }
    }
}

void PersistenceProgram::processUnmatchedOrderMessage(const Json::Value& message) {
    Json::Value orderData = deserializeMessage(message["order"].asString());
    if (!orderData.isObject()) {
        LOG_ERROR("Invalid order data: not an object");
        throw std::runtime_error("Invalid order data");
    }
    Order order = deserializeOrder(orderData);
    processOrder(order);
}

void PersistenceProgram::processTradeMessage(const Json::Value& message) {
    Json::Value buyOrderData = deserializeMessage(message["buyOrder"].asString());
    Json::Value sellOrderData = deserializeMessage(message["sellOrder"].asString());
    Json::Value tradeRecordData = deserializeMessage(message["tradeRecord"].asString());

    if (!buyOrderData.isObject() || !sellOrderData.isObject() || !tradeRecordData.isObject()) {
        LOG_ERROR("Invalid trade message data: not an object");
        throw std::runtime_error("Invalid trade message data");
    }

    Order buyOrder = deserializeOrder(buyOrderData);
    Order sellOrder = deserializeOrder(sellOrderData);
    TradeRecord trade = deserializeTradeRecord(tradeRecordData);

    executeQuery("START TRANSACTION");
    processOrder(buyOrder);
    processOrder(sellOrder);
    processTradeRecord(trade);
    executeQuery("COMMIT");
}

bool PersistenceProgram::executeQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(connMutex);
    return dbConn->executeQuery(query);
}

void PersistenceProgram::processOrder(const Order& order) {
    std::string status = "MATCHING";
    if(order.filledQuantity >= order.quantity){
        status = "FULLY_FILLED";
    }else if(order.filledQuantity > 0){
        status = "PARTIALLY_FILLED";
    }

    std::string query = "UPDATE orders SET status = '" + status +
                        "', filled_quantity = " + std::to_string(order.filledQuantity) +
                        " WHERE order_id = " + std::to_string(order.orderId);
    executeQuery(query);
}

void PersistenceProgram::processTradeRecord(const TradeRecord& trade) {
    std::string query = "INSERT INTO trade_records (buyer_user_id, seller_user_id, buyer_order_id, seller_order_id, order_type, trade_price, trade_quantity, buyer_fee, seller_fee) VALUES (" +
                        std::to_string(trade.buyerUserId) + ", " + std::to_string(trade.sellerUserId) + ", " +
                        std::to_string(trade.buyerOrderId) + ", " + std::to_string(trade.sellerOrderId) + ", '" + trade.orderType + "', " +
                        std::to_string(trade.tradePrice) + ", " + std::to_string(trade.tradeQuantity) + ", " + std::to_string(trade.buyerFee) + ", " +
                        std::to_string(trade.sellerFee) + ")";

    executeQuery(query);
}
