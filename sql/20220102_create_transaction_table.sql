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
  `comment` varchar(100) NOT NULL,
  PRIMARY KEY (`timestamp`,`index`)
);

CREATE VIEW `v_tx` AS
    SELECT 
        `transaction`.`timestamp` AS `timestamp`,
        `transaction`.`index` AS `index`,
        `transaction`.`from` AS `from`,
        `transaction`.`log_count` AS `log_count`,
        `transaction`.`tx_fee` AS `tx_fee`,
        `transaction`.`gas_price` AS `gas_price`,
        `transaction`.`block_number` AS `block_number`,
        `transaction`.`delta_msec` AS `delta_msec`,
        `transaction`.`bot_id` AS `bot_id`,
        `transaction`.`hash` AS `hash`,
		`transaction`.`comment` AS `comment`
    FROM
        `transaction`;
