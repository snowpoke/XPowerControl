#pragma once
#include "stdafx.h"
#include <string>
#include <curl/curl.h>
#include "wstring_transform.h"
#include "http_requests.h"

using namespace std;

//  libcurl write callback function
static int http_requests::writer(char* data, size_t size, size_t nmemb,
	string* writerData)
{
	if (writerData == NULL)
		return 0;

	writerData->append(data, size * nmemb);

	return static_cast<int>(size * nmemb);
}

string http_requests::load_page(const string& link_t, const string& SESSID_t) {
	CURL* curl;
	CURLcode res;
	string ret = "";
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, link_t.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
		string cookie_val = "iksm_session=" + SESSID_t + ";";
		curl_easy_setopt(curl, CURLOPT_COOKIE, cookie_val.c_str());

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK) {
			AfxMessageBox((L"Failed to load information from URL " + s2ws(link_t)).c_str());
			AfxThrowUserException();
		}


		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return ret;
}
