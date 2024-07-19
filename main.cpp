#include <thread>
#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include "OrderGenerator.h"
#include "MatchingEngine.h"
#include "PersistenceProgram.h"
#include "HealthCheckServer.h"
#include "DbConfig.h"
#include "DbConnection.h"
#include "Logger.h"

// 全局日志输出流
namespace {
    std::ofstream logFile("server.log", std::ios::app);
    std::mutex logMutex;
}

void startMessageQueueServersAndMatchingEngine() {
    // 创建消息队列
    zmq::context_t context(1);
    zmq::socket_t orderSocket(context, zmq::socket_type::pull);
    orderSocket.bind("tcp://*:12345");

    zmq::socket_t resultSocket(context, zmq::socket_type::push);
    resultSocket.bind("tcp://*:12346");

    zmq::socket_t bookSocket(context, zmq::socket_type::pub);
    bookSocket.bind("tcp://*:12347");

    // 启动撮合引擎
    MatchingEngine matchingEngine(orderSocket, resultSocket, bookSocket);
    std::thread matchingEngineThread([&matchingEngine]() {
        try {
            matchingEngine.start();
        } catch (const std::exception& e) {
            LOG_INFO("Error in MatchingEngine: " + std::string(e.what()));
        }
    });

    // 等待撮合引擎线程结束
    matchingEngineThread.join();
}

void startPersistenceProgram(const DbConfig& config) {
    DbConnection dbConn(config);

    // 创建 ZeroMQ 上下文
    zmq::context_t context(1);

    // 启动持久化程序
    PersistenceProgram persistenceProgram(dbConn, context, "tcp://localhost:12346");
    persistenceProgram.start();

    // 持续运行持久化程序
    while (persistenceProgram.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


void startOrderGenerator(const DbConfig& config) {
    DbConnection dbConn(config);

    // 创建 ZeroMQ 上下文
    zmq::context_t context(1);

    // 启动订单生成器
    OrderGenerator orderGenerator(dbConn, context, "tcp://localhost:12345");
    orderGenerator.generateOrders(100);
}


void startHeal() {
    // 启动健康检查服务器
    // 创建 ZeroMQ 上下文
    zmq::context_t context(1);
    HealthCheckServer server("localhost", 8080, context, "tcp://localhost:12347");
    server.start();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <component> <log_file>" << std::endl;
        std::cerr << "Components: message_queue_servers, persistence_program, order_generator, matching_engine, health_check_server" << std::endl;
        return 1;
    }

    std::string component = argv[1];

    // 重定向 std::cout 和 std::cerr 到日志文件
    std::streambuf* coutBuf = std::cout.rdbuf();
    std::streambuf* cerrBuf = std::cerr.rdbuf();
    std::cout.rdbuf(logFile.rdbuf());
    std::cerr.rdbuf(logFile.rdbuf());

    Logger::getInstance().init("server.log", LogLevel::INFO);

    LOG_INFO("Starting " + component);
    try {
        // 读取数据库配置
        DbConfig config = readConfig("config.json");

        if (component == "match") {
            startMessageQueueServersAndMatchingEngine();
        } else if (component == "persis") {
            startPersistenceProgram(config);
        } else if (component == "order") {
            startOrderGenerator(config);
        } else if (component == "heal") {
            startHeal();
        } else {
            std::cerr << "Unknown component: " << component << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        LOG_INFO("Exception: " + std::string(e.what()));
    } catch (...) {
        LOG_INFO("Unknown exception.");
    }

    LOG_INFO("Trading System stopped.");

    // 恢复 std::cout 和 std::cerr 的缓冲区
    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);

    logFile.close();
    return 0;
}
