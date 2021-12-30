#include <uint256.h>
#include <HexCoding.h>

#include <Ethereum/Transaction.h>
#include <Ethereum/ABI/Function.h>
#include <Ethereum/ABI/ParamAddress.h>
#include <Ethereum/Signer.h>

#include "binacpp/binacpp.h"

#include <easylogging++.h>
#include <jsoncpp/json/json.h>

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

INITIALIZE_EASYLOGGINGPP

// testnet Compound
const std::string URL = "https://data-seed-prebsc-1-s1.binance.org:8545/";
const int chainID = 97;
const std::string WALLET = "9777d3d77f4cc000ed6aa6979854178a28e04eb5";
const std::string SECRET = "b7cf1604a1f1f57765ee9613ea5188ac077ab82155d87f749c86e39eaec53c15";
const std::string CONTRACT = "cd5dc972dbc4df70f64871d87ae8f64d32988279";  // BNB token


BinaCPP rest(URL);

static const std::vector<std::string> headers {
	"Content-Type: application/json"
};

std::string pretty_print(const Json::Value& val)
{
	Json::StreamWriterBuilder builder;
	builder.settings_["indentation"] = "";
	return Json::writeString(builder, val);
}

Json::Value make_json_rpc(const std::string& method)
{
	Json::Value doc;
	doc["method"] = method;
	doc["id"] = 1;
	doc["jsonrpc"] = "2.0";
	return doc;
}

Json::Value rest_request(Json::Value doc)
{
	std::string request = pretty_print(doc);

	LOG(DEBUG) << "Request: " << request;

	std::string str_result;
	rest.curl_api_with_header(URL, str_result, headers, request, "POST");

	LOG(DEBUG) << "Response: " << str_result;

	Json::Reader reader;
	Json::Value json_result;
	reader.parse(str_result, json_result);

	return json_result;
}

Json::Value eth_getTransactionCount(const std::string& address)
{
	auto doc = make_json_rpc("eth_getTransactionCount");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + address);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value eth_gasPrice()
{
	auto doc = make_json_rpc("eth_gasPrice");

	Json::Value arr = Json::arrayValue;
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value eth_estimateGas()
{
	auto doc = make_json_rpc("eth_estimateGas");

	Json::Value params = Json::objectValue;
	Json::Value arr = Json::arrayValue;
	arr.append(params);
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value eth_getBalance(const std::string& address)
{
	auto doc = make_json_rpc("eth_getBalance");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + address);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	return rest_request(doc);
}

Json::Value eth_call(const std::string& from, const std::string& to, const std::string& data)
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

Json::Value eth_sendRawTransaction(const std::string& from, const std::string& to, const std::string& data)
{
	auto doc = make_json_rpc("eth_sendRawTransaction");

	Json::Value arr = Json::arrayValue;
	arr.append("0x" + data);
	doc["params"] = arr;

	return rest_request(doc);
}

TW::uint256_t hexToUInt256(std::string s)
{
	if (s.length() % 2)
		s.insert(s.begin() + 2, '0');
	TW::Ethereum::ABI::ParamUInt256 result;
	result.setValueJson(s);
	return result.getVal();
}

TW::uint256_t nonce256 = 0;
auto toAddress = TW::parse_hex(CONTRACT);

std::string prepare_transaction()
{
	try {

		auto fromAddress = TW::parse_hex(WALLET);

		auto approveFunc = TW::Ethereum::ABI::Function("approve", std::vector<std::shared_ptr<TW::Ethereum::ABI::ParamBase>>{
			std::make_shared<TW::Ethereum::ABI::ParamAddress>(fromAddress),
			std::make_shared<TW::Ethereum::ABI::ParamUInt256>(0)
		});
		auto approveSig = TW::hex(approveFunc.getSignature());

		auto response = eth_getTransactionCount(WALLET);
		nonce256 = hexToUInt256(response["result"].asString());

		response = eth_gasPrice();
		auto gasPrice256 = hexToUInt256(response["result"].asString());

		response = eth_estimateGas();
		auto gasLimit256 = hexToUInt256(response["result"].asString());

		LOG(DEBUG) << "nonce = " << nonce256
					<< ", gasPrice = " << gasPrice256
					<< ", gasLimit = " << gasLimit256;

		//response = eth_call(WALLET, CONTRACT, approveSig);
		//response = eth_getBalance(WALLET);

		auto privateKey = TW::PrivateKey(TW::parse_hex(SECRET));

		TW::Data payload;
		approveFunc.encode(payload);

		auto transaction = std::make_shared<TW::Ethereum::TransactionNonTyped>(nonce256, gasPrice256, gasLimit256, toAddress, 0, payload);
	
		auto signature = TW::Ethereum::Signer::sign(privateKey, chainID, transaction);

    	auto encoded = transaction->encoded(signature, chainID);

		auto data = TW::hex(encoded);

		//response = eth_call(WALLET, CONTRACT, data);
		response = eth_sendRawTransaction(WALLET, CONTRACT, data);

		return data;
	}
	catch (const std::exception& ex) {
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	return "";
}

boost::asio::io_service io;
boost::posix_time::seconds interval(10);
auto start = boost::posix_time::from_time_t(1640906520);
boost::asio::deadline_timer timer(io, start);

int counter = 5;

void timer_cb(const boost::system::error_code& /*e*/)
{
	LOG(DEBUG) << "Hello, world!";
	
	--counter;

	if (counter > 0) {
		timer.expires_at(timer.expires_at() + interval);
		// Posts the timer event
		timer.async_wait(timer_cb);
	}
}

int main(int argc, char* argv[])
{
	START_EASYLOGGINGPP(argc, argv);
	el::Configurations defaultConf;
	defaultConf.setToDefault();
	defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime | %msg");
	el::Loggers::reconfigureLogger("default", defaultConf);

	rest.init("", "");

	auto data = prepare_transaction();

	timer.async_wait(timer_cb);
	io.run();

	return 0;
}
