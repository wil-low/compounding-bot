#include "Bot.h"
#include "binacpp/binacpp.h"

#include <HexCoding.h>

#include <Ethereum/Transaction.h>
#include <Ethereum/ABI/Function.h>
#include <Ethereum/ABI/ParamAddress.h>
#include <Ethereum/Signer.h>

#include <easylogging++.h>
#include <boost/date_time/posix_time/posix_time.hpp>

const std::vector<std::string> Bot::headers_ {
	"Content-Type: application/json"
};

Bot::Bot(Json::Value& config, boost::asio::io_service& io)
: config_(config)
, url_(config["url"].asString())
, chain_id_(config["chain_id"].asInt())
, nonce_(0)
, gas_price_(0)
, gas_limit_(0)
, nearest_compounding_time_(0)
, contract_(TW::parse_hex(config["contract"].asString()))
, wallet_hex_(config["wallet"].asString())
, private_key_(TW::parse_hex(config["secret"].asString()))
, compound_timer_(io)
{
	LOG(DEBUG) << "Created Bot #" << config_["id"].asInt() << ": " << config_["name"].asString();
	wallet_ = TW::parse_hex(wallet_hex_);
	rest_ = new BinaCPP(url_);
	rest_->init("", "");
}

Bot::~Bot()
{
	LOG(DEBUG) << "Deleted Bot #" << config_["id"].asInt();
	delete rest_;
}

std::string Bot::pretty_print(const Json::Value& val, bool indent)
{
	Json::StreamWriterBuilder builder;
	if (!indent)
		builder.settings_["indentation"] = "";
	return Json::writeString(builder, val);
}

Json::Value Bot::parse_json(const std::string& str_result)
{
	Json::Reader reader;
	Json::Value json_result;
	if (!reader.parse(str_result, json_result)) {
		throw std::logic_error("Parse error: " + str_result);
	}
	return json_result;
}

Json::Value Bot::make_json_rpc(const std::string& method)
{
	Json::Value doc;
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

Json::Value Bot::rest_request(Json::Value doc)
{
	std::string request = pretty_print(doc);

	LOG(DEBUG) << "Request: " << request;

	std::string str_result;
	rest_->curl_api_with_header(url_, str_result, headers_, request, "POST");

	LOG(DEBUG) << "Response: " << str_result;

	Json::Reader reader;
	Json::Value json_result;
	reader.parse(str_result, json_result);

	return json_result;
}

Json::Value Bot::eth_getTransactionCount(const std::string& address)
{
	auto doc = make_json_rpc("eth_getTransactionCount");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + address);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value Bot::eth_gasPrice()
{
	auto doc = make_json_rpc("eth_gasPrice");

	Json::Value arr = Json::arrayValue;
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value Bot::eth_estimateGas()
{
	auto doc = make_json_rpc("eth_estimateGas");

	Json::Value params = Json::objectValue;
	Json::Value arr = Json::arrayValue;
	arr.append(params);
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value Bot::eth_getBalance(const std::string& address)
{
	auto doc = make_json_rpc("eth_getBalance");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + address);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value Bot::eth_call(const std::string& from, const std::string& to, const std::string& data)
{
	auto doc = make_json_rpc("eth_call");

	Json::Value params;
	params["from"] = "0x" + from;
	params["to"] = "0x" + to;
	params["data"] = "0x" + data;
	Json::Value arr = Json::arrayValue;
	arr.append(params);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value Bot::eth_sendRawTransaction(const std::string& data)
{
	auto doc = make_json_rpc("eth_sendRawTransaction");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + data);
	doc["params"] = arr;

	return rest_request(doc);
}

void Bot::prepare_transaction()
{
	auto approveFunc = TW::Ethereum::ABI::Function("approve", std::vector<std::shared_ptr<TW::Ethereum::ABI::ParamBase>>{
		std::make_shared<TW::Ethereum::ABI::ParamAddress>(wallet_),
		std::make_shared<TW::Ethereum::ABI::ParamUInt256>(0)
	});
	auto approveSig = TW::hex(approveFunc.getSignature());

	auto response = eth_getTransactionCount(wallet_hex_);
	nonce_ = hexToUInt256(response["result"].asString());

	response = eth_gasPrice();
	gas_price_ = hexToUInt256(response["result"].asString());

	response = eth_estimateGas();
	gas_limit_ = hexToUInt256(response["result"].asString());

	LOG(DEBUG) << "nonce = " << nonce_
				<< ", gas_price = " << gas_price_
				<< ", gas_limit = " << gas_limit_;

	TW::Data payload;
	approveFunc.encode(payload);

	auto transaction = std::make_shared<TW::Ethereum::TransactionNonTyped>(nonce_, gas_price_, gas_limit_, contract_, 0, payload);
	auto signature = TW::Ethereum::Signer::sign(private_key_, chain_id_, transaction);
	auto encoded = transaction->encoded(signature, chain_id_);
	prepared_tx_ = TW::hex(encoded);
	//response = eth_sendRawTransaction(prepared_tx_);
}

void Bot::compound_timer_cb(const boost::system::error_code& /*e*/)
{
	LOG(DEBUG) << "compound_timer_cb start";
	auto response = eth_sendRawTransaction(prepared_tx_);
	LOG(DEBUG) << "compound_timer_cb end";
	
	/*--counter;

	if (counter > 0) {
		compound_timer_.expires_at(compound_timer_.expires_at() + interval);
		// Posts the timer event
		compound_timer_.async_wait(std::bind(&Bot::compound_timer_cb, this, std::placeholders::_1);
	}*/
}

void Bot::start()
{
	compound_timer_.cancel();
	prepare_transaction();

	auto start = boost::posix_time::from_time_t(config_["start_time"].asInt());
	compound_timer_.expires_at(start);
	compound_timer_.async_wait(std::bind(&Bot::compound_timer_cb, this, std::placeholders::_1));
}
