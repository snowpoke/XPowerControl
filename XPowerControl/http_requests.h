#pragma once
#include <string>
#include <curl/curl.h>

namespace http_requests {
	//  libcurl write callback function
	static int writer(char* data, size_t size, size_t nmemb, std::string* writerData);

	// sends http get request with iksm_token cookie
	std::string load_page(const std::string& link_t, const std::string& SESSID_t);
}