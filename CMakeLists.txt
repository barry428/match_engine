cmake_minimum_required(VERSION 3.25)
project(TradingSystem)

set(CMAKE_CXX_STANDARD 17)

# 设置目标架构为 arm64
set(CMAKE_OSX_ARCHITECTURES "arm64")

# 设置 macOS 目标版本
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

# 设置 jsoncpp 的路径
set(JSONCPP_INCLUDE_DIR /opt/homebrew/opt/jsoncpp/include)
set(JSONCPP_LIB_DIR /opt/homebrew/opt/jsoncpp/lib)

include_directories(${JSONCPP_INCLUDE_DIR})
link_directories(${JSONCPP_LIB_DIR})

# 手动设置 MySQL 库和头文件路径
set(MySQL_INCLUDE_DIRS /usr/local/mysql/include)
set(MySQL_LIBRARIES /usr/local/mysql/lib)

include_directories(${MySQL_INCLUDE_DIRS})
link_directories(${MySQL_LIBRARIES})

# httpLib
include_directories(third_party/cpp-httplib-master)

# 查找 ZeroMQ 库
find_path(ZeroMQ_INCLUDE_DIR NAMES zmq.hpp PATHS /opt/homebrew/include)
find_library(ZeroMQ_LIBRARY NAMES zmq PATHS /opt/homebrew/lib)

if(NOT ZeroMQ_INCLUDE_DIR)
    message(FATAL_ERROR "ZeroMQ include directory not found. Please install ZeroMQ and set the correct paths.")
else()
    message(STATUS "ZeroMQ include directory found: ${ZeroMQ_INCLUDE_DIR}")
endif()

if(NOT ZeroMQ_LIBRARY)
    message(FATAL_ERROR "ZeroMQ library not found. Please install ZeroMQ and set the correct paths.")
else()
    message(STATUS "ZeroMQ library found: ${ZeroMQ_LIBRARY}")
endif()

include_directories(${ZeroMQ_INCLUDE_DIR})

# 添加 WebSocket++ 依赖项
include_directories(third_party/websocketpp)

# 查找 OpenSSL 库
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_directories(${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL not found. Please install OpenSSL and set the correct paths.")
endif()

# 添加 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)


# 添加可执行文件
add_executable(TradingSystem main.cpp src/Serialization.cpp src/OrderGenerator.cpp src/MatchingEngine.cpp src/PersistenceProgram.cpp src/HealthCheckServer.cpp src/DbConfig.cpp src/DbConnection.cpp src/Order.cpp include/Logger.h src/WebSocketServer.cpp include/WebSocketServer.h)

# 链接 Boost、MySQL 和 jsoncpp 库
target_link_libraries(TradingSystem mysqlclient jsoncpp ${ZeroMQ_LIBRARY} OpenSSL::SSL OpenSSL::Crypto)

# debug cmake option
# -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
# -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

