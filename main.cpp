#include "Bot.h"

#include <easylogging++.h>

#include <iostream>
#include <string>
#include <boost/asio.hpp>

INITIALIZE_EASYLOGGINGPP

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

int main(int argc, char* argv[])
{
	START_EASYLOGGINGPP(argc, argv);
	el::Configurations defaultConf;
	defaultConf.setToDefault();
	defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime | %msg");
	el::Loggers::reconfigureLogger("default", defaultConf);

	LOG(INFO) << "=======================";
	LOG(INFO) << "compounding-bot started";

	if (argc > 1) {
		Json::Value cfg = load_config(argv[1]);

		Bot bot(cfg, io);

		bot.init();
		bot.start();

		io.run();
	}
	else {
		LOG(ERROR) << "Usage: ./compounding-bot <config.json>";
	}

	LOG(INFO) << "compounding-bot finished\n\n";
	return 0;
}
