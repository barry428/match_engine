#include "DbConfig.h"
#include <fstream>
#include <sstream>
#include <json/json.h>

DbConfig readConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFile);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string configContent = buffer.str();

    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    DbConfig config;

    if (Json::parseFromStream(readerBuilder, buffer, &root, &errs)) {
        config.host = root["database"]["host"].asString();
        config.port = root["database"]["port"].asInt();
        config.user = root["database"]["user"].asString();
        config.password = root["database"]["password"].asString();
        config.database = root["database"]["database"].asString();
    } else {
        throw std::runtime_error("Failed to parse configuration file: " + errs);
    }

    return config;
}
