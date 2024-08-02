#include "DbConnectionPool.h"
#include "Logger.h"
#include <iostream>

DbConnectionPool::DbConnectionPool(const DbConfig& config, size_t poolSize) : poolSize(poolSize) {
    for (size_t i = 0; i < poolSize; ++i) {
        DbConnection* conn = new DbConnection(config);
        if (conn->getConnection() == nullptr) {
            LOG_ERROR("Failed to create database connection.");
            delete conn;
            throw std::runtime_error("Failed to create database connection");
        }
        connections.push_back(conn);
        availableConnections.push(conn);
    }
}

DbConnectionPool::~DbConnectionPool() {
    for (auto conn : connections) {
        delete conn;
    }
}

DbConnection* DbConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(poolMutex);
    condition.wait(lock, [this]() { return !availableConnections.empty(); });

    DbConnection* conn = availableConnections.front();
    availableConnections.pop();
    return conn;
}

void DbConnectionPool::returnConnection(DbConnection* conn) {
    std::lock_guard<std::mutex> lock(poolMutex);
    availableConnections.push(conn);
    condition.notify_one();
}
