#pragma once

#include "DbConnection.h"
#include "DbConfig.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class DbConnectionPool {
public:
    DbConnectionPool(const DbConfig& config, size_t poolSize);
    ~DbConnectionPool();

    DbConnection* getConnection();
    void returnConnection(DbConnection* conn);

private:
    std::vector<DbConnection*> connections;
    std::queue<DbConnection*> availableConnections;
    std::mutex poolMutex;
    std::condition_variable condition;
    size_t poolSize;
};
