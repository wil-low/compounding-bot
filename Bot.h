#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

#include <uint256.h>

class BinaCPP;

namespace TW {
	class PrivateKey;
	namespace Ethereum {
		namespace ABI {
			class Function;
		}
	}
}

class Bot
{
public:
	Bot(nlohmann::json& config, boost::asio::io_service& io);
	~Bot();

	void init();
	void start();

	static std::string pretty_print(const nlohmann::json& val, bool indent = false);
	static nlohmann::json parse_json(const std::string& str_result);
	static nlohmann::json make_json_rpc(const std::string& method);
	static TW::uint256_t hexToUInt256(std::string s);

	void timer_cb(const boost::system::error_code& /*e*/);
	void cooldown_cb(const boost::system::error_code& /*e*/);  // after bounty

private:
	static const std::vector<std::string> headers_;

	void check_config(const std::string& tag, std::string& output);
	void check_config(const std::string& tag, int& output);
	void check_config(const std::string& tag, TW::uint256_t& output);

	void prepare_transaction(TW::Ethereum::ABI::Function* func);
	nlohmann::json rest_request(nlohmann::json doc);

	nlohmann::json eth_getTransactionCount(const std::string& address);
	nlohmann::json eth_gasPrice();
	nlohmann::json eth_estimateGas();
	nlohmann::json eth_getBalance(const std::string& address);
	nlohmann::json eth_call(const std::string& from, const std::string& to, const std::string& data);
	nlohmann::json eth_sendRawTransaction(const std::string& data);

	void schedule_for_10x1min();
	void schedule_for_compound_time();

	void log_schedule();

	BinaCPP* rest_;

	nlohmann::json config_;

	std::string mode_;
	std::string url_;
	int chain_id_;

	TW::uint256_t nonce_;
	TW::uint256_t gas_price_;
	TW::uint256_t gas_limit_;

	TW::uint256_t nearest_compounding_time_;

	std::string prepared_tx_;
	std::string last_tx_hash_;

	std::string contract_hex_;
	TW::Data contract_;

	std::string wallet_hex_;
	TW::Data wallet_;

	TW::PrivateKey* private_key_;

	TW::Ethereum::ABI::Function *approve_func_;
	TW::Ethereum::ABI::Function *compound_func_;
	TW::Ethereum::ABI::Function *nearestCompoundingTime_func_;

	boost::asio::deadline_timer main_timer_;
	boost::posix_time::milliseconds delta_msec_;
};
