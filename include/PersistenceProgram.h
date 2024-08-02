#pragma once

#include "Order.h"
#include "Serialization.h"
#include "TradeRecord.h"
#include "DbConnection.h"
#include "DbConnectionPool.h"
#include "Logger.h"
#include <thread>
#include <boost/asio.hpp>
#include <mutex>
#include <zmq.hpp>

class PersistenceProgram {
public:
    PersistenceProgram(DbConnectionPool& connectionPool, zmq::context_t& context, const std::string& resultServerAddress);
    ~PersistenceProgram(); // Destructor to release connection
    void start();
    void stop();
    bool isRunning() const { return running; }

private:
    void run();
    void reconnectResultClient();
    void processMessage(const Json::Value& message);
    void processUnmatchedOrderMessage(const Json::Value& message);
    void processTradeMessage(const Json::Value& message);
    bool executeQuery(const std::string& query);
    void processOrder(const Order& order);
    void processTradeRecord(const TradeRecord& trade);

    DbConnectionPool& dbConnPool;
    DbConnection* dbConn; // Pointer to DbConnection
    zmq::context_t& context;
    std::string resultServerAddress;
    zmq::socket_t resultSocket;
    std::atomic<bool> running;
    std::thread workerThread;
    std::mutex connMutex;
    MYSQL* conn;
};
