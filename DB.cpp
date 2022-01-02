#include "DB.h"
#include "Transaction.h"

#include <easylogging++.h>
#include <mysql.h>

class Transaction;

DB::DB()
: mysql_(nullptr)
{
	if (mysql_library_init(0, NULL, NULL)) {
		LOG(ERROR) << "Could not initialize MySQL client library";
		exit(1);
	}
}

DB::~DB()
{
	bool close_fail = mysql_stmt_close (insert_tx_stmt_);
	if (close_fail) {
		LOG(ERROR) << mysql_error(mysql_);
	}
	if (mysql_)
		mysql_close(mysql_);
	mysql_library_end();
}

void DB::connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& db)
{
	const char INSERT_TX_QUERY[] = "insert into transaction "
		"(`timestamp`, `index`, `from`, `to`, `log_count`, `tx_fee`, `hash`, `block_number`, `gas_limit`, `gas_price`, `gas_used`, `status`, `bot_id`, `delta_msec`) "
		"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	mysql_ = mysql_init(nullptr);
	if (mysql_real_connect(mysql_, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), 0, NULL, 0)) {
		LOG(DEBUG) << "DB connection OK";
	}
	else {
		LOG(ERROR) << "Cannot connect to DB: " << mysql_error(mysql_);
		exit(1);
	}

	insert_tx_stmt_ = mysql_stmt_init (mysql_);
	if (NULL == insert_tx_stmt_)
		exit (EXIT_FAILURE); /* OUT OF MEMORY */

	bool prepare_fail = mysql_stmt_prepare (insert_tx_stmt_, INSERT_TX_QUERY, strlen(INSERT_TX_QUERY));
	if (prepare_fail) {
		LOG(ERROR) << mysql_stmt_error(insert_tx_stmt_);
		exit(1);
	}
}

enum DB_insert_params
{
	param_timestamp,
	param_index,
	param_from,
	param_to,
	param_log_count,
	param_tx_fee,
	param_hash,
	param_block_number,
	param_gas_limit,
	param_gas_price,
	param_gas_used,
	param_status,
	param_bot_id,
	param_delta_msec,
	param_max
};

void DB::store_tx(const Transaction& tr)
{
	MYSQL_BIND bind[param_max] = {0};

	memset(bind, 0, sizeof(bind));

	bind[param_timestamp].buffer_type = MYSQL_TYPE_LONG;
	bind[param_timestamp].buffer = (char*)&tr.timestamp_;

	bind[param_index].buffer_type = MYSQL_TYPE_LONG;
	bind[param_index].buffer = (char*)&tr.index_;

	unsigned long from_len = tr.from_.size() + 1;
	bind[param_from].buffer_type = MYSQL_TYPE_STRING;
	bind[param_from].buffer = (char*)tr.from_.c_str();
	bind[param_from].buffer_length = from_len;
	bind[param_from].length = &from_len;

	unsigned long to_len = tr.to_.size() + 1;
	bind[param_to].buffer_type = MYSQL_TYPE_STRING;
	bind[param_to].buffer = (char*)tr.to_.c_str();
	bind[param_to].buffer_length = to_len;
	bind[param_to].length = &to_len;

	bind[param_log_count].buffer_type = MYSQL_TYPE_LONG;
	bind[param_log_count].buffer = (char*)&tr.log_count_;

	bind[param_tx_fee].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[param_tx_fee].buffer = (char*)&tr.tx_fee_;

	unsigned long hash_len = tr.hash_.size() + 1;
	bind[param_hash].buffer_type = MYSQL_TYPE_STRING;
	bind[param_hash].buffer = (char*)tr.hash_.c_str();
	bind[param_hash].buffer_length = hash_len;
	bind[param_hash].length = &hash_len;

	bind[param_block_number].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[param_block_number].buffer = (char*)&tr.block_number_;

	bind[param_gas_limit].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[param_gas_limit].buffer = (char*)&tr.gas_limit_;

	bind[param_gas_price].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[param_gas_price].buffer = (char*)&tr.gas_price_;

	bind[param_gas_used].buffer_type = MYSQL_TYPE_LONGLONG;
	bind[param_gas_used].buffer = (char*)&tr.gas_used_;

	bind[param_status].buffer_type = MYSQL_TYPE_LONG;
	bind[param_status].buffer = (char*)&tr.status_;

	bind[param_bot_id].buffer_type = MYSQL_TYPE_LONG;
	bind[param_bot_id].buffer = (char*)&tr.bot_id_;

	bind[param_delta_msec].buffer_type = MYSQL_TYPE_LONG;
	bind[param_delta_msec].buffer = (char*)&tr.delta_msec_;

	bool bind_failed = mysql_stmt_bind_param(insert_tx_stmt_, bind);
	if (bind_failed) {
		LOG(ERROR) << mysql_stmt_error(insert_tx_stmt_);
		exit(1);
	}

	bool execute_fail = mysql_stmt_execute(insert_tx_stmt_);
	if (execute_fail) {
		LOG(ERROR) << mysql_stmt_error(insert_tx_stmt_);
		exit(1);
	}
}
