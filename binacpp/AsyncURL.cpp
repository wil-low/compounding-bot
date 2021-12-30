/***************************************************************************
*
*			Copyright (c) 2010-2020  Argo SE, Inc. All rights reserved.
*
* The following source code, and the ideas, expressions and concepts
* incorporated therein, as well as the ideas, expressions and concepts of
* any other material (whether tangible or intangible) disclosed in
* conjunction with this source code, are the exclusive and proprietary
* property, and constitutes confidential information, of Argo SE, Inc.
* Accordingly, the user of this source code
* agrees that it shall continue to be bound by the terms and conditions of
* any and all confidentiality agreements executed with Argo SE, Inc.
* concerning the disclosure of the source code and any other materials in
* conjunction therewith.
*
*/

#include "AsyncURL.h"
#include <curl/curl.h>
#include <json/json.h>
#include <loglib/OpenLog.h>
using namespace loglib;

bool AsyncURL::inited_ = false;

int run_thread(void* arg)
{
	MDP_TRC(enAdmin, enTraceDebug, "%D | %s\n", __FUNCTION__);
	auto au = (AsyncURL*)arg;
	auto cm = au->curl_multi();

	CURLMsg *msg;
	int msgs_left = -1;
	int still_alive = 1;
	long return_http_code = 0;

	do {
		curl_multi_perform(cm, &still_alive);

		while (msg = curl_multi_info_read(cm, &msgs_left)) {
			if (msg->msg == CURLMSG_DONE) {
				AsyncURL::Callback* cb;
				CURL *eh = msg->easy_handle;
				curl_easy_getinfo(eh, CURLINFO_PRIVATE, &cb);
				curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &return_http_code);
				cb->handler_->response_cb(return_http_code, cb->data_);
				delete cb;
				curl_multi_remove_handle(cm, eh);
				curl_easy_cleanup(eh);
			}
			else {
				fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
			}
		}
		if (still_alive)
			curl_multi_wait(cm, NULL, 0, 1000, NULL);

	} while (still_alive);

	MDP_TRC(enAdmin, enTraceDebug, "%D | %s finished\n", __FUNCTION__);
	return 0;
}

AsyncURL::Callback::Callback(AsyncURL_Handler* handler)
: headers_(nullptr)
, handler_(handler)
{
}

AsyncURL::Callback::~Callback()
{
	curl_slist_free_all(headers_);
}


AsyncURL::AsyncURL()
{
}

AsyncURL::~AsyncURL()
{
}

void AsyncURL::stop()
{
	curl_multi_cleanup(curl_);
	if (inited_)
		curl_global_cleanup();
}

void AsyncURL::start()
{
	if (!inited_) {
		curl_global_init(CURL_GLOBAL_DEFAULT);
		inited_ = true;
	}
	curl_ = curl_multi_init();
	thread_ = new std::thread(run_thread, this);
}

std::size_t AsyncURL::write_cb(char *content, std::size_t size, std::size_t nmemb, void *buffer)
{
	//MDP_TRC(enAdmin, enTraceDebug, "%D | %s\n", __FUNCTION__) ;
	auto tgt = static_cast<std::string*>(buffer);

	tgt->append(content, size * nmemb);

	//MDP_TRC(enAdmin, enTraceDebug, "%D | %s done\n", __FUNCTION__) ;
	return size * nmemb;
}

void AsyncURL::request(const std::string& url, AsyncURL_Handler* handler, const std::vector<std::string>& extra_http_headers, const std::string& post_data, const std::string& action)
{
	//MDP_TRC(enAdmin, enTraceDebug, "%D | %s\n", __FUNCTION__) ;
	if (curl_) {
		CURL *eh = curl_easy_init();
		auto cb = new Callback(handler);
		curl_easy_setopt(eh, CURLOPT_URL, url);
		curl_easy_setopt(eh, CURLOPT_PRIVATE, cb);
		curl_easy_setopt(eh, CURLOPT_WRITEDATA, &cb->data_);
		curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
		curl_easy_setopt(eh, CURLOPT_FAILONERROR, 0);
		curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(eh, CURLOPT_ENCODING, "gzip");

		if (!extra_http_headers.empty()) {
			for (auto& hdr : extra_http_headers)
				cb->headers_ = curl_slist_append(cb->headers_, hdr.c_str());
			curl_easy_setopt(eh, CURLOPT_HTTPHEADER, cb->headers_);
		}

		if (!post_data.empty() || action == "POST" || action == "PUT" || action == "DELETE") {
			if (action == "PUT" || action == "DELETE" || action == "POST")
				curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, action.c_str());
			curl_easy_setopt(eh, CURLOPT_POSTFIELDS, post_data.c_str());
		}

		curl_multi_add_handle(curl_, eh);
	}
}