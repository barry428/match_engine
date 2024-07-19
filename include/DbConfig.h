#pragma once

#include <string>

struct DbConfig {
    std::string host;
    int port;
    std::string user;
    std::string password;
    std::string database;
};

DbConfig readConfig(const std::string& configFile);
