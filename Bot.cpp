#include "Bot.h"
#include "binacpp/binacpp.h"

#include <HexCoding.h>
#include <PrivateKey.h>

#include <Ethereum/Transaction.h>
#include <Ethereum/ABI/Function.h>
#include <Ethereum/ABI/ParamAddress.h>
#include <Ethereum/Signer.h>

#include <easylogging++.h>
#include <boost/date_time/posix_time/posix_time.hpp>


const char MODE_approve10x1min[] = "approve10x1min";
const char MODE_compound10x1min[] = "compound10x1min";
const char MODE_compound[] = "compound";

const std::vector<std::string> Bot::headers_ {
	"Content-Type: application/json"
};

Bot::Bot(nlohmann::json& config, asio::io_service& io)
: config_(config)
, rest_(nullptr)
, nonce_(0)
, gas_price_(0)
, gas_limit_(0)
, nearest_compounding_time_(0)
, private_key_(nullptr)
, main_timer_(io)
, delta_msec_(0)
, approve_func_(nullptr)
, compound_func_(nullptr)
, nearestCompoundingTime_func_(nullptr)
{
}

Bot::~Bot()
{
	delete rest_;
	delete private_key_;
	delete approve_func_;
	delete compound_func_;
	delete nearestCompoundingTime_func_;

	LOG(DEBUG) << "Deleted Bot #" << config_["id"];
}

void Bot::check_config(const std::string& tag, std::string& output)
{
	if (config_[tag].is_string() && !config_[tag].empty())
		output = config_[tag];
	else
		throw std::invalid_argument("check_config: string tag '" + tag + "' not found");
}

void Bot::check_config(const std::string& tag, int& output)
{
	if (config_[tag].is_number())
		output = config_[tag];
	else
		throw std::invalid_argument("check_config: int tag '" + tag + "' not found");
}

void Bot::init()
{
	int id, start_time, delta;
	std::string name, contract, secret;

	check_config("id", id);
	check_config("name", name);
	check_config("mode", mode_);
	check_config("url", url_);
	check_config("chain_id", chain_id_);
	check_config("contract", contract_hex_);
	check_config("wallet", wallet_hex_);
	check_config("secret", secret);
	check_config("start_time", start_time);
	check_config("delta_msec", delta);

	delta_msec_ = boost::posix_time::milliseconds(delta);

	LOG(DEBUG) << "Init Bot #" << id << ": " << name;

	contract_ = TW::parse_hex(contract_hex_);
	private_key_ = new TW::PrivateKey(TW::parse_hex(secret));
	wallet_ = TW::parse_hex(wallet_hex_);
	rest_ = new BinaCPP(url_);
	rest_->init("", "");

	approve_func_ = new TW::Ethereum::ABI::Function("approve", std::vector<std::shared_ptr<TW::Ethereum::ABI::ParamBase>>{
		std::make_shared<TW::Ethereum::ABI::ParamAddress>(wallet_),
		std::make_shared<TW::Ethereum::ABI::ParamUInt256>(0)
	});

	compound_func_ = new TW::Ethereum::ABI::Function("compound");
	nearestCompoundingTime_func_ = new TW::Ethereum::ABI::Function("nearestCompoundingTime");

	auto response = eth_getTransactionCount(wallet_hex_);
	nonce_ = hexToUInt256(response["result"]);

	LOG(DEBUG) << "nonce = " << nonce_;
}

std::string Bot::pretty_print(const nlohmann::json& val, bool indent)
{
	if (indent)
		return val.dump(4);
	return val.dump();
}

nlohmann::json Bot::parse_json(const std::string& str_result)
{
	auto json_result = nlohmann::json::parse(str_result);
	if (json_result.is_discarded()) {
		throw std::logic_error("Parse error: " + str_result);
	}
	return json_result;
}

nlohmann::json Bot::make_json_rpc(const std::string& method)
{
	nlohmann::json doc;
	doc["method"] = method;
	doc["id"] = 1;
	doc["jsonrpc"] = "2.0";
	return doc;
}

TW::uint256_t Bot::hexToUInt256(std::string s)
{
	if (s.length() % 2)
		s.insert(s.begin() + 2, '0');
	TW::Ethereum::ABI::ParamUInt256 result;
	result.setValueJson(s);
	return result.getVal();
}

nlohmann::json Bot::rest_request(nlohmann::json doc)
{
	std::string request = pretty_print(doc);

	LOG(DEBUG) << "Request: " << request;

	std::string str_result;
	rest_->curl_api_with_header(url_, str_result, headers_, request, "POST");

	LOG(DEBUG) << "Response: " << str_result;

	return parse_json(str_result);
}

nlohmann::json Bot::eth_getTransactionCount(const std::string& address)
{
	auto doc = make_json_rpc("eth_getTransactionCount");

	auto arr = nlohmann::json::array();
	arr.push_back("0x" + address);
	arr.push_back(nlohmann::json("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

nlohmann::json Bot::eth_gasPrice()
{
	auto doc = make_json_rpc("eth_gasPrice");

	auto arr = nlohmann::json::array();
	doc["params"] = arr;

	return rest_request(doc);
}

nlohmann::json Bot::eth_estimateGas()
{
	auto doc = make_json_rpc("eth_estimateGas");

	auto params = nlohmann::json::object();
	auto arr = nlohmann::json::array();
	arr.push_back(params);
	doc["params"] = arr;

	return rest_request(doc);
}

nlohmann::json Bot::eth_getBalance(const std::string& address)
{
	auto doc = make_json_rpc("eth_getBalance");

	auto arr = nlohmann::json::array();
	arr.push_back("0x" + address);
	arr.push_back(nlohmann::json("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

nlohmann::json Bot::eth_call(const std::string& from, const std::string& to, const std::string& data)
{
	auto doc = make_json_rpc("eth_call");

	nlohmann::json params;
	params["from"] = "0x" + from;
	params["to"] = "0x" + to;
	params["data"] = "0x" + data;
	nlohmann::json arr = nlohmann::json::array();
	arr.push_back(params);
	arr.push_back(nlohmann::json("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

nlohmann::json Bot::eth_sendRawTransaction(const std::string& data)
{
	auto doc = make_json_rpc("eth_sendRawTransaction");

	auto arr = nlohmann::json::array();
	arr.push_back("0x" + data);
	doc["params"] = arr;

	return rest_request(doc);
}

void Bot::prepare_transaction(TW::Ethereum::ABI::Function* func)
{
	auto sig = TW::hex(func->getSignature());

	auto response = eth_gasPrice();
	gas_price_ = hexToUInt256(response["result"]);
	gas_price_ *= 10;

	response = eth_estimateGas();
	gas_limit_ = hexToUInt256(response["result"]);
	gas_limit_ *= 2;

	LOG(DEBUG) << "nonce = " << nonce_
				<< ", gas_price = " << gas_price_
				<< ", gas_limit = " << gas_limit_;

	TW::Data payload;
	func->encode(payload);

	auto transaction = std::make_shared<TW::Ethereum::TransactionNonTyped>(nonce_++, gas_price_, gas_limit_, contract_, 0, payload);
	auto signature = TW::Ethereum::Signer::sign(*private_key_, chain_id_, transaction);
	auto encoded = transaction->encoded(signature, chain_id_);
	prepared_tx_ = TW::hex(encoded);
	//response = eth_sendRawTransaction(prepared_tx_);
}

void Bot::schedule_for_10x1min()
{
	auto start = boost::posix_time::from_time_t(config_["start_time"]);
	start += delta_msec_;

	if (start < boost::posix_time::second_clock::universal_time())
		throw std::logic_error("start_time in the past");

	main_timer_.expires_at(start + delta_msec_);
	main_timer_.async_wait(std::bind(&Bot::timer_cb, this, std::placeholders::_1));
	log_schedule();
}

void Bot::schedule_for_compound_time()
{
	auto response = eth_call(wallet_hex_, contract_hex_, TW::hex(nearestCompoundingTime_func_->getSignature()));
	auto next = hexToUInt256(response["result"]);
	LOG(DEBUG) << "next = " << next;

	if (nearest_compounding_time_ != next) {
		nearest_compounding_time_ = next;
		auto start = boost::posix_time::from_time_t((time_t)next);
		main_timer_.expires_at(start + delta_msec_);
		main_timer_.async_wait(std::bind(&Bot::timer_cb, this, std::placeholders::_1));
	}
	else {
		// reschedule for 20 sec after bounty distribution
		main_timer_.expires_at(boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(20));
		main_timer_.async_wait(std::bind(&Bot::cooldown_cb, this, std::placeholders::_1));
	}
	log_schedule();
}

void Bot::log_schedule()
{
	LOG(DEBUG) << "timer_cb scheduled for " << mode_ << " at " << to_simple_string(main_timer_.expires_at()) << " UTC";
}

void Bot::timer_cb(const std::error_code& /*e*/)
{
	if (mode_ == MODE_approve10x1min) {
		static int counter = 10;
		static const boost::posix_time::minutes interval(1);

		LOG(DEBUG) << "timer_cb approve #" << counter << " start";
		auto response = eth_sendRawTransaction(prepared_tx_);
		LOG(DEBUG) << "timer_cb approve #" << counter << " end";

		--counter;

		if (counter > 0) {
			prepare_transaction(approve_func_);
			main_timer_.expires_at(main_timer_.expires_at() + interval);
			main_timer_.async_wait(std::bind(&Bot::timer_cb, this, std::placeholders::_1));
			log_schedule();
		}
	}
	else if (mode_ == MODE_compound10x1min) {
		static int counter = 10;
		static const boost::posix_time::minutes interval(1);

		LOG(DEBUG) << "timer_cb compound #" << counter << " start";
		auto response = eth_sendRawTransaction(prepared_tx_);
		LOG(DEBUG) << "timer_cb compound #" << counter << " end";

		--counter;

		if (counter > 0) {
			prepare_transaction(compound_func_);
			main_timer_.expires_at(main_timer_.expires_at() + interval);
			main_timer_.async_wait(std::bind(&Bot::timer_cb, this, std::placeholders::_1));
			log_schedule();
		}
	}
	else if (mode_ == MODE_compound) {
		LOG(DEBUG) << "timer_cb compound start";
		auto response = eth_sendRawTransaction(prepared_tx_);
		LOG(DEBUG) << "timer_cb compound end";

		prepare_transaction(compound_func_);
		schedule_for_compound_time();
	}
}

void Bot::cooldown_cb(const std::error_code& /*e*/)
{
	if (mode_ == MODE_compound) {
		LOG(DEBUG) << "cooldown_cb compound";
		schedule_for_compound_time();
	}
}

void Bot::start()
{
	main_timer_.cancel();

	if (mode_ == MODE_approve10x1min) {
		prepare_transaction(approve_func_);
		schedule_for_10x1min();
	}
	else if (mode_ == MODE_compound10x1min) {
		prepare_transaction(compound_func_);
		schedule_for_10x1min();
	}
	else if (mode_ == MODE_compound) {
		prepare_transaction(compound_func_);
		schedule_for_compound_time();
	}
	else {
		throw std::logic_error("Unknown mode: " + mode_);
	}
}
