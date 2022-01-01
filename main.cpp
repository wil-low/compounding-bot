#include "Bot.h"

#include <HexCoding.h>
#include <Coin.h>
#include <Keystore/StoredKey.h>

#include <easylogging++.h>

#include <iostream>
#include <string>
#include <asio.hpp>

#include <openssl/crypto.h>

INITIALIZE_EASYLOGGINGPP

const TWCoinType coin_type = TWCoinType::TWCoinTypeEthereum;
const int PASSWORD_LEN = 100;

asio::io_service io;

nlohmann::json load_config(const std::string& fn)
{
	std::ifstream in(fn);
	if (!in) {
		LOG(ERROR) << "Cannot open config file: " << fn;
		exit(1);
	}
	std::stringstream buffer;
	buffer << in.rdbuf();
	in.close();
	LOG(DEBUG) << "Contents of " << fn << ": " << buffer.str();
	return Bot::parse_json(buffer.str());
}

void store_wallet(const std::string& keystore, const std::string& name, const std::string& private_key, const std::string& password)
{
	TW::PrivateKey priv(TW::parse_hex(private_key));
	TW::Data pass(password.begin(), password.end());
	auto stored_key = TW::Keystore::StoredKey::createWithPrivateKey(name, pass, priv.bytes);
	stored_key.store(keystore);
}

TW::PrivateKey load_wallet(const std::string& keystore, const char* password)
{
	TW::Data pass(password, password + strlen(password));
	OPENSSL_cleanse((void*)password, PASSWORD_LEN);
	auto stored_key = TW::Keystore::StoredKey::load(keystore);
	return stored_key.privateKey(coin_type, pass);
}

int main(int argc, char* argv[])
{
	START_EASYLOGGINGPP(argc, argv);
	el::Configurations defaultConf;
	defaultConf.setToDefault();
	defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime | %msg");
	el::Loggers::reconfigureLogger("default", defaultConf);

	try {
		LOG(INFO) << "=======================";
		LOG(INFO) << "compounding-bot started";

		if (argc > 1) {
			nlohmann::json cfg = load_config(argv[1]);

			bool keys_present = cfg["secret"].is_string() && !cfg["secret"].empty() && cfg["wallet"].is_string() && !cfg["wallet"].empty();
			if (!keys_present) {
				auto password = getpass("Enter password for keystore: ");
				auto privateKey = load_wallet("bot.keystore", password);

				cfg["secret"] = TW::hex(privateKey.bytes);
				cfg["wallet"] = TW::deriveAddress(coin_type, privateKey).substr(2);
				//LOG(INFO) << cfg["secret"].asString();
				//LOG(INFO) << cfg["wallet"].asString();
			}

			Bot bot(cfg, io);

			bot.init();
			bot.start();

			io.run();
		}
		else {
			LOG(ERROR) << "Usage: ./compounding-bot <config.json>";
		}

		LOG(INFO) << "compounding-bot finished\n\n";
	}
	catch (TW::Keystore::DecryptionError&) {
		LOG(ERROR) << "DecryptionError";
	}
	catch (std::exception& e) {
		LOG(ERROR) << "Exception: " << e.what();
	}
	return 0;
}
