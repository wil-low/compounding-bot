CREATE TABLE `transaction` (
  `timestamp` int NOT NULL,
  `index` int NOT NULL,
  `from` varchar(100) NOT NULL,
  `to` varchar(100) NOT NULL,
  `log_count` int NOT NULL,
  `tx_fee` double NOT NULL,
  `hash` varchar(100) NOT NULL,
  `block_number` bigint NOT NULL,
  `gas_limit` bigint NOT NULL,
  `gas_price` bigint NOT NULL,
  `gas_used` bigint NOT NULL,
  `status` int NOT NULL,
  `bot_id` int NOT NULL,
  `delta_msec` int NOT NULL,
  PRIMARY KEY (`timestamp`,`index`)
);

