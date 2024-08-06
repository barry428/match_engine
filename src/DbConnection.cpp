#include "DbConnection.h"
#include <iostream>

DbConnection::DbConnection(const DbConfig& config) {
    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        LOG_WARN("mysql_init() failed.");
        return;
    }

    if (mysql_real_connect(conn, config.host.c_str(), config.user.c_str(), config.password.c_str(), config.database.c_str(), 0, nullptr, 0) == nullptr) {
        LOG_WARN("mysql_real_connect() failed.");
        mysql_close(conn);
        conn = nullptr;
    }
}

DbConnection::~DbConnection() {
    if (conn) {
        mysql_close(conn);
    }
}

bool DbConnection::executeQuery(const std::string& query) {
    LOG_DEBUG("Executing query: " + query);
    if (mysql_query(conn, query.c_str())) {
        LOG_DEBUG("Query failed: " + std::string(mysql_error(conn)));
        if (mysql_errno(conn) == CR_SERVER_GONE_ERROR || mysql_errno(conn) == CR_SERVER_LOST) {
            reconnect();
            if (mysql_query(conn, query.c_str())) {
                LOG_WARN("Query failed after reconnect: " + std::string(mysql_error(conn)));
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

std::vector<std::map<std::string, std::string>> DbConnection::executeQueryWithResult(const std::string& query) {
    std::vector<std::map<std::string, std::string>> results;
    LOG_DEBUG("Executing query with result: " + query);
    if (mysql_query(conn, query.c_str())) {
        LOG_DEBUG("Query failed: " + std::string(mysql_error(conn)));
        if (mysql_errno(conn) == CR_SERVER_GONE_ERROR || mysql_errno(conn) == CR_SERVER_LOST) {
            reconnect();
            if (mysql_query(conn, query.c_str())) {
                LOG_ERROR("Query failed after reconnect: " + std::string(mysql_error(conn)));
                return results;
            }
        } else {
            return results;
        }
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        LOG_WARN("mysql_store_result() failed: " + std::string(mysql_error(conn)));
        return results;
    }

    int num_fields = mysql_num_fields(res);
    MYSQL_ROW row;
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> result_row;
        for (int i = 0; i < num_fields; ++i) {
            result_row[fields[i].name] = row[i] ? row[i] : "NULL";
        }
        results.push_back(result_row);
    }

    mysql_free_result(res);
    return results;
}


void DbConnection::reconnect() {
    LOG_DEBUG("Reconnecting to database...");
    mysql_close(conn);
    conn = getConnection();
    if (conn == nullptr) {
        LOG_ERROR("Failed to reconnect to database.");
        throw std::runtime_error("Failed to reconnect to database");
    }
    LOG_DEBUG("Reconnected to database successfully");
}
