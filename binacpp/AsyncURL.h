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

#pragma once

#include <string>
#include <vector>
#include <thread>

class AsyncURL_Handler {
public:
	virtual void response_cb(int http_code, const std::string& data) = 0;
};

typedef void CURLM;
struct curl_slist;

class AsyncURL {
public:
	AsyncURL();
	virtual ~AsyncURL();

	void start();
	void stop();

	void request(const std::string& url, AsyncURL_Handler* handler, const std::vector<std::string>& extra_http_headers, const std::string& post_data, const std::string& action);

	CURLM* curl_multi() { return curl_; }

	struct Callback {
		Callback(AsyncURL_Handler* handler);
		~Callback();
		AsyncURL_Handler* handler_;
		std::string data_;
		curl_slist* headers_;
	};

private:
	static std::size_t write_cb(char *content, std::size_t size, std::size_t nmemb, void *buffer);

	CURLM* curl_ {nullptr};
	static bool inited_;
	std::thread* thread_;
};
