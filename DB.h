#pragma once

#include <string>

class Transaction;
struct MYSQL;
struct MYSQL_STMT;

class DB
{
public:
	DB();
	~DB();

	void connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& db);
	void store_tx(const Transaction& tr);

private:
	MYSQL* mysql_;
	MYSQL_STMT* insert_tx_stmt_;
};
