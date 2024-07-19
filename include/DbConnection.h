#pragma once

#include <mysql.h>
#include <string>
#include <vector>
#include <map>
#include "DbConfig.h"
#include "Logger.h"

class DbConnection {
public:
    DbConnection(const DbConfig& config);
    ~DbConnection();

    MYSQL* getConnection() { return conn; }
    bool executeQuery(const std::string& query);
    std::vector<std::map<std::string, std::string>> executeQueryWithResult(const std::string& query);
    void reconnect();

private:
    MYSQL* conn;
};
