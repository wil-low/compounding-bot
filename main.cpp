#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWAnySigner.h>
#include <TrustWalletCore/TWCoinTypeConfiguration.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWString.h>

#include <uint256.h>
#include <HexCoding.h>
#include <proto/Ethereum.pb.h>
#include <TrustWalletCore/TWEthereumAbi.h>
#include <TrustWalletCore/TWEthereumAbiFunction.h>

#include "binacpp/binacpp.h"

#include <easylogging++.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>

INITIALIZE_EASYLOGGINGPP

const std::string URL = "https://data-seed-prebsc-1-s1.binance.org:8545/";
BinaCPP rest(URL);


std::string pretty_print(const Json::Value& val)
{
	Json::StreamWriterBuilder builder;
	builder.settings_["indentation"] = "";
	return Json::writeString(builder, val);
}

std::string eth_call(const std::string& from, const std::string& to, const std::string& data)
{
	static const std::vector<std::string> headers {
		"Content-Type: application/json"
	};

	Json::Value doc;
	doc["method"] = "eth_call";
	doc["id"] = 1;
	doc["jsonrpc"] = "2.0";

	Json::Value params;
	params["from"] = "0x" + from;
	params["to"] = "0x" + to;
	params["data"] = "0x" + data;
	Json::Value arr = Json::arrayValue;
	arr.append(params);
	arr.append(Json::Value("latest"));
	doc["params"] = arr;

	std::string request = pretty_print(doc);

	LOG(DEBUG) << "Request: " << request;

	std::string str_result;
	rest.curl_api_with_header(URL, str_result, headers, request, "POST");

	LOG(DEBUG) << "Response: " << str_result;
	return str_result;
}

int main(int argc, char* argv[])
{
	START_EASYLOGGINGPP(argc, argv);
	LOG(INFO) << "My first info log using default logger";
	try {

		const std::string WALLET = "9777d3d77f4cc000ed6aa6979854178a28e04eb5";
		const std::string SECRET = "b7cf1604a1f1f57765ee9613ea5188ac077ab82155d87f749c86e39eaec53c15";
		const std::string CONTRACT = "cd5dc972dbc4df70f64871d87ae8f64d32988279";

		rest.init("", "");

		auto funSymbol = TWEthereumAbiEncode(TWEthereumAbiFunctionCreateWithString(TWStringCreateWithUTF8Bytes("symbol")));
		auto s = TWStringCreateWithHexData(funSymbol);
		std::string data = TWStringUTF8Bytes(s);

		auto response = eth_call(WALLET, CONTRACT, data);


		auto chainId = TW::store(TW::uint256_t(1));
		auto nonce = TW::store(TW::uint256_t(0));
		auto gasPrice = TW::store(TW::uint256_t(42000000000)); // 0x09c7652400
		auto gasLimit = TW::store(TW::uint256_t(78009)); // 130B9
		auto toAddress = "0x5322b34c88ed0691971bf52a7047448f0f4efc84";
		auto token = "0x6b175474e89094c44da98b954eedeac495271d0f"; // DAI
		auto amount = TW::uint256_t(0000000000000000000);
		auto amountData = TW::store(amount);
		auto key = TW::parse_hex("0x608dcb1742bb3fb7aec002074e3420e4fab7d00cced79ccdac53ed5b27138151");

		//TW::Ethereum::Proto::Transaction_ContractGeneric input;
		//input.set_amount(amountData.data(), amountData.size());
		//input.set_data()  (funSymbol, funSymbol);



		// Steps for sending:
		// 1. put together a send message (contains sender and receiver address, amount, gas price, etc.)
		// 2. sign this message
		// 3. broadcast this message to the P2P network -- not done in this sample
		// Note that Signer input and output are represented as protobuf binary messages, for which support is missing in C++.
		// Therefore some direct serialization/parsing is done in helper methods.
		
		/*std::cout << "SEND funds:" << std::endl;
		const std::string dummyReceiverAddress = "0xC37054b3b48C3317082E7ba872d7753D13da4986";
		auto secretPrivKey = TWPrivateKeyData(secretPrivateKeyDefault);

		std::cout << "preparing transaction (using AnySigner) ... ";
		std::string chainIdB64 = base64(parse_hex("01"))
		std::string gasPriceB64 = "1pOkAA==";   // base64(parse_hex("d693a4")) decimal 3600000000
		std::string gasLimitB64 = "Ugg=";	   // base64(parse_hex("5208")) decimal 21000
		std::string amountB64 = "A0i8paFgAA=="; // base64(parse_hex("0348bca5a160"))  924400000000000
		std::string transaction = "{"
			"\"chainId\":\"" + chainIdB64 +
			"\",\"gasPrice\":\"" + gasPriceB64 +
			"\",\"gasLimit\":\"" + gasLimitB64 +
			"\",\"toAddress\":\"" + dummyReceiverAddress +
			"\",\"transaction\":{\"transfer\":{\"amount\":\"" + amountB64 +
			"\"}}}";			
		std::cout << "transaction: " << transaction << std::endl;

		std::cout << "signing transaction ... ";

		auto json = TWStringCreateWithUTF8Bytes(transaction.c_str());
		auto result = TWAnySignerSignJSON(json, secretPrivKey, TWCoinTypeEthereum);
		auto signedTransaction = std::string(TWStringUTF8Bytes(result));
		std::cout << "done" << std::endl;
		std::cout << "Signed transaction data (to be broadcast to network):  (len " << signedTransaction.length() << ") '" << signedTransaction << "'" << std::endl;
		// see e.g. https://github.com/flightwallet/decode-eth-tx for checking binary output content
		std::cout << std::endl;
		TWStringDelete(json);
		TWStringDelete(result);*/
		std::cout << "Bye!" << std::endl;
	}
	catch (const std::exception& ex) {
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
}
