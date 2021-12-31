#include "Bot.h"

#include <HexCoding.h>
#include <Coin.h>
#include <Keystore/StoredKey.h>

#include <easylogging++.h>

#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include <openssl/crypto.h>

INITIALIZE_EASYLOGGINGPP

const TWCoinType coin_type = TWCoinType::TWCoinTypeEthereum;
const int PASSWORD_LEN = 128;

boost::asio::io_service io;

Json::Value load_config(const std::string& fn)
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
			Json::Value cfg = load_config(argv[1]);

			bool keys_present = cfg["secret"].isString() && !cfg["secret"].asString().empty() && cfg["wallet"].isString() && !cfg["wallet"].asString().empty();
			if (!keys_present) {
				auto password = getpass("Enter password for keystore: ");
				auto privateKey = load_wallet("bot.keystore", password);

				cfg["secret"] = TW::hex(privateKey.bytes);
				cfg["wallet"] = TW::deriveAddress(coin_type, privateKey);
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
		LOG(ERROR) << "Caught DecryptionError";
	}
	catch (std::exception& e) {
		LOG(ERROR) << "Caught exception: " << e.what();
	}
	return 0;
}
