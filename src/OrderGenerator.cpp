#include "OrderGenerator.h"
#include "Serialization.h"  // 假设我们有一个序列化库
#include <chrono>
#include <thread>

std::atomic<unsigned int> OrderGenerator::orderIdCounter(10000);

OrderGenerator::OrderGenerator(DbConnection& dbConn, zmq::context_t& context, const std::string& orderServerAddress)
        : orderSocket(context, zmq::socket_type::push), dbConn(dbConn), generator(std::random_device()()),
          priceDistribution(5000.0, 60000.0),
          quantityDistribution(0.1, 10.0),
          feeRateDistribution(0.001, 0.005),
          orderSideDistribution(0, 1),
          orderTypeDistribution(0, 1) {
    orderSocket.connect(orderServerAddress);
}

void OrderGenerator::generateOrders(int numOrders) {
    loadOrdersFromDatabase();

    orderIdCounter = getMaxOrderId();
    for (int i = 0; i < numOrders; ++i) {
        Order order = createRandomOrder();
        sendOrder(order, false);

        // 模拟订单生成的延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

Order OrderGenerator::createRandomOrder() {
    Order order;
    order.orderId = ++orderIdCounter;
    order.userId = orderIdCounter;
    order.price = roundToPrecision(priceDistribution(generator), 8);
    order.quantity = roundToPrecision(quantityDistribution(generator), 6);
    order.feeRate = roundToPrecision(feeRateDistribution(generator), 6);
    order.orderSide = getRandomOrderSide();
    // order.orderType = getRandomOrderType();
    order.orderType = OrderType::LIMIT;
    order.status = OrderStatus::INITIAL;
    order.createTime = std::chrono::system_clock::now();
    order.updateTime = order.createTime;
    order.filledQuantity = 0.0;
    LOG_DEBUG("create random orders from database. orderId:" + std::to_string(order.orderId) + " price:" + std::to_string(order.price));
    return order;
}

OrderSide OrderGenerator::getRandomOrderSide() {
    return static_cast<OrderSide>(orderSideDistribution(generator));
}

OrderType OrderGenerator::getRandomOrderType() {
    return static_cast<OrderType>(orderTypeDistribution(generator));
}

void OrderGenerator::sendOrder(const Order& order, bool isUpdate) {
    Json::Value message;
    if(!isUpdate){
        writeOrderToDatabase(order);
    }

    message["type"] = "ORDER";
    message["order"] = serializeOrder(order);  // Use the new serializeOrder function
    std::string serializedMessage = serializeMessage(message);  // Serialize the message

    zmq::message_t zmqMessage(serializedMessage.data(), serializedMessage.size());
    orderSocket.send(zmqMessage, zmq::send_flags::none);
}

void OrderGenerator::writeOrderToDatabase(const Order& order) {
    // 数据库写入逻辑
    std::string query = "INSERT INTO orders (order_id, user_id, price, quantity, fee_rate, order_side, order_type, status, filled_quantity) VALUES (" +
                        std::to_string(order.orderId) + ", " + std::to_string(order.userId) + ", " + std::to_string(order.price) + ", " +
                        std::to_string(order.quantity) + ", " + std::to_string(order.feeRate) + ", '" + orderSideToString(order.orderSide) + "', '" +
                        orderTypeToString(order.orderType) + "', '" + orderStatusToString(order.status) + "', " + std::to_string(order.filledQuantity) + ")";

    if (!dbConn.executeQuery(query)) {
        LOG_DEBUG("Failed to insert order into database");
    }
}

void OrderGenerator::loadOrdersFromDatabase() {
    std::string query = "SELECT order_id, user_id, price, quantity, fee_rate, order_side, order_type, status, filled_quantity, create_time, update_time FROM orders WHERE status IN ('INITIAL', 'MATCHING', 'PARTIALLY_FILLED') ORDER BY create_time ASC";

    try {
        std::vector<std::map<std::string, std::string>> results = dbConn.executeQueryWithResult(query);

        if (results.empty()) {
            LOG_DEBUG("No orders found with status INITIAL, MATCHING, or PARTIALLY_FILLED.");
            return;
        }

        double total = 0;
        for (const auto& row : results) {
            Order order;
            order.orderId = std::stoul(row.at("order_id"));
            order.userId = std::stoul(row.at("user_id"));
            order.price = std::stod(row.at("price"));
            order.quantity = std::stod(row.at("quantity"));
            order.feeRate = std::stod(row.at("fee_rate"));
            order.orderSide = stringToOrderSide(row.at("order_side"));
            order.orderType = stringToOrderType(row.at("order_type"));
            order.status = stringToOrderStatus(row.at("status"));
            order.filledQuantity = std::stod(row.at("filled_quantity"));
            order.createTime = string_to_time_point(row.at("create_time"));
            order.updateTime = string_to_time_point(row.at("update_time"));
            LOG_DEBUG("loading orders from database. orderId:" + std::to_string(order.orderId) + " price:" + std::to_string(order.price) + " price:" + row.at("price"));
            sendOrder(order, true);
            total += order.quantity - order.filledQuantity;
        }
        LOG_DEBUG("loading orders from database. total:" + std::to_string(total));
    } catch (const std::exception& e) {
        LOG_DEBUG("Error loading orders from database: " + std::string(e.what()));
        // 这里可以根据需要进行进一步的错误处理
    }
}


unsigned int OrderGenerator::getMaxOrderId() {
    std::string query = "SELECT MAX(order_id) AS max_order_id FROM orders";
    auto results = dbConn.executeQueryWithResult(query);
    try {
        if (!results.empty()) {
            return std::stoul(results[0].at("max_order_id"));
        }
    } catch (const std::exception& e) {
        LOG_DEBUG("Error loading orders from database: " + std::string(e.what()));
    }

    return 10000;
}

double OrderGenerator::roundToPrecision(double value, int precision) {
    double factor = std::pow(10.0, precision);
    return std::round(value * factor) / factor;
}
