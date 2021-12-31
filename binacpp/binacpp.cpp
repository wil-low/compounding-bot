#pragma warning(disable: 4996)

#include "binacpp.h"
#include <curl/curl.h>
#include <easylogging++.h>

BinaCPP::BinaCPP(const std::string & host_address)
	: host_address_(host_address)
{

}

BinaCPP::~BinaCPP()
{
	curl_easy_cleanup(curl_);
}

void BinaCPP::init(const std::string &api_key, const std::string &secret_key )
{
	curl_ = curl_easy_init();
	if (curl_ == NULL)
	{
		LOG(ERROR) << "BinaCPP::init ERROR! curl_easy_init returned NULL";
	}
	api_key_ = api_key;
	secret_key_ = secret_key;
}

//-----------------
// Curl's callback
std::size_t BinaCPP::curl_cb(char *content, std::size_t size, std::size_t nmemb, void *buffer)
{	
	//LOG(DEBUG) << __FUNCTION__;
	auto tgt = static_cast<std::string*>(buffer);

	tgt->append(content, size*nmemb);

	//LOG(DEBUG) << __FUNCTION__ << " done.";
	return size*nmemb;
}

void BinaCPP::curl_api(const std::string &url, std::string& result, const std::string & action, const std::string &post_data)
{
	std::vector <std::string> v;
	std::string str_result;
	curl_api_with_header(host_address_ + url, result, v, post_data, action);
}

//--------------------
// Do the curl
int BinaCPP::curl_api_with_header(const std::string &url, std::string &str_result, const std::vector <std::string> &extra_http_header, const std::string &post_data, const std::string &action)
{
	std::lock_guard<std::mutex> g(mutex_);
	//LOG(DEBUG) << "%s\n", __FUNCTION__) ;
	CURLcode res;
	long return_http_code = 0;

	if(curl_) {
		curl_easy_reset(curl_);
		curl_easy_setopt(curl_, CURLOPT_URL, url.c_str() );
		curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, BinaCPP::curl_cb);
		curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &str_result );
		curl_easy_setopt(curl_, CURLOPT_FAILONERROR, 0);
		curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl_, CURLOPT_ENCODING, "gzip");
		curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl_, CURLOPT_TCP_NODELAY, 1);

		struct curl_slist *chunk = nullptr;
		if (!extra_http_header.empty()) {
			for (auto& h : extra_http_header) {
				chunk = curl_slist_append(chunk, h.c_str());
			}
			curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, chunk);
		}

		curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, action.c_str());
		if (post_data.size() > 0)
			curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, post_data.c_str() );

		res = curl_easy_perform(curl_);

		if (!extra_http_header.empty()) {
			curl_slist_free_all(chunk);
		}

		/* Check for errors */ 
		curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &return_http_code);
		if (return_http_code < 400)  // not an error
			return_http_code = 0;
		else
			LOG(DEBUG) << "curl_easy_perform() failed, response code " << return_http_code;
	}
	return return_http_code;
	//LOG(DEBUG) << "%s done\n", __FUNCTION__);
}
