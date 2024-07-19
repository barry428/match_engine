CREATE TABLE `orders` (
                          `order_id` bigint NOT NULL AUTO_INCREMENT,
                          `user_id` bigint NOT NULL,
                          `order_side` enum('BUY','SELL') NOT NULL,
                          `price` decimal(18,8) NOT NULL,
                          `quantity` decimal(10,6) NOT NULL,
                          `fee_rate` decimal(5,4) NOT NULL,
                          `trading_pair` varchar(20) NOT NULL DEFAULT 'BTC_USDT',
                          `status` enum('INITIAL','MATCHING','PARTIALLY_FILLED','FULLY_FILLED','CANCELING','CANCELED','EXCEPTION') NOT NULL,
                          `order_type` enum('MARKET','LIMIT') NOT NULL,
                          `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
                          `update_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                          `filled_quantity` decimal(10,6) NOT NULL DEFAULT '0.000000',
                          PRIMARY KEY (`order_id`),
                          KEY `idx_user_id` (`user_id`),
                          KEY `idx_status` (`status`),
                          KEY `idx_trading_pair` (`trading_pair`)
) ENGINE=InnoDB AUTO_INCREMENT=10101 DEFAULT CHARSET=utf8mb3;


CREATE TABLE `orders` (
                          `order_id` bigint NOT NULL AUTO_INCREMENT,
                          `user_id` bigint NOT NULL,
                          `order_side` enum('BUY','SELL') NOT NULL,
                          `price` decimal(18,8) NOT NULL,
                          `quantity` decimal(10,6) NOT NULL,
                          `fee_rate` decimal(5,4) NOT NULL,
                          `trading_pair` varchar(20) NOT NULL DEFAULT 'BTC_USDT',
                          `status` enum('INITIAL','MATCHING','PARTIALLY_FILLED','FULLY_FILLED','CANCELING','CANCELED','EXCEPTION') NOT NULL,
                          `order_type` enum('MARKET','LIMIT') NOT NULL,
                          `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
                          `update_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                          `filled_quantity` decimal(10,6) NOT NULL DEFAULT '0.000000',
                          PRIMARY KEY (`order_id`),
                          KEY `idx_user_id` (`user_id`),
                          KEY `idx_status` (`status`),
                          KEY `idx_trading_pair` (`trading_pair`)
) ENGINE=InnoDB AUTO_INCREMENT=10101 DEFAULT CHARSET=utf8mb3