#ifndef BINACPP_H
#define BINACPP_H


#include <string>
#include <vector>
#include <mutex>

namespace Json {
	class Value;
}
typedef void CURL;

class BinaCPP {
public:
	BinaCPP(const std::string & host_address);
	virtual ~BinaCPP();

	void init(const std::string &api_key, const std::string &secret_key);

	void curl_api(const std::string &url, std::string& json_result, const std::string & action, const std::string &post_data);
	int curl_api_with_header(const std::string &url, std::string &str_result, const std::vector <std::string> &extra_http_header, const std::string &post_data, const std::string &action);

	std::string host_address_;
	std::string api_key_;
	std::string secret_key_;

	CURL* curl_handler() { return curl_;  }

protected:
	static std::size_t curl_cb(char *content, std::size_t size, std::size_t nmemb, void *buffer);

	CURL* curl_ {nullptr};
	std::mutex mutex_;
};


#endif
