#pragma once
#include "stdafx.h"
#include <string>
#include <curl/curl.h>
#include "wstring_transform.h"
#include "http_requests.h"
#include "logging.h"
#include "CustomDialogBase.h"
#include <boost/algorithm/string.hpp>
#include "nlohmann/json.hpp"

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
	auto _deflogger = logging::get_logger(DEFAULT_LOG);
	auto _jsonlogger = logging::get_logger(JSON_LOG);

	CURL* curl;
	CURLcode res;
	bool res_ok = false;
	string ret = "";
	int attempt_num = 1;
	while (res_ok == false && attempt_num <= 3) {
		attempt_num++;
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


			/* always cleanup */
			curl_easy_cleanup(curl);

			res_ok = (res == CURLE_OK); // retry up to three times if there's an issue
		}
	}

	/* Check for errors */
	if (res != CURLE_OK) {
		AfxMessageBox((L"Failed to load information from URL " + transform::s2ws(link_t)).c_str());
		AfxThrowUserException();
	}

	
	_deflogger->info("Downloaded page from {} with iksm_session={} and returned response of {} characters.",
		link_t, SESSID_t, ret.length());
	_jsonlogger->info("Downloaded page from {} with iksm_session={} and returned response: {}",
		link_t, SESSID_t, ret);

	return ret;
}

std::string http_requests::get_global_info(const std::string& key_t)
{
	string version_page = http_requests::load_page("https://raw.githubusercontent.com/snowpoke/XPowerControl-public/master/version.json", "");
	boost::erase_all(version_page, "\n");
	auto j = nlohmann::json::parse(version_page);
	return j[key_t];
}

UINT http_requests::thread_download_file(LPVOID pParam)
{
	DownloadParam* dl_param = (DownloadParam*)pParam;
	auto _logger = logging::get_logger(DEFAULT_LOG);

	_logger->info("Initiating Download from {} to {}.", transform::ws2s(dl_param->dl_URL), transform::ws2s(dl_param->savepath_t));
	URLDownloadToFile(NULL,
		dl_param->dl_URL.data(),
		(dl_param->savepath_t).c_str(),
		0,
		dl_param->callback_t);

	return 0;
}

// our implementation of the IBindStatusCallback interface

http_requests::Callback::Callback()
{
	Callback(nullptr);
}

http_requests::Callback::Callback(CustomDialogBase* dlg_t)
{
	dlg = dlg_t;
	_logger = logging::get_logger(DEFAULT_LOG);
}

void http_requests::Callback::cancel()
{
	cancel_download = true; // download is cancelled by returning E_ABORT from any callback function
}

HRESULT http_requests::Callback::GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) {
	_logger->info("Called GetBindInfo");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::GetPriority(long* pnPriority) {
	_logger->info("Called GetPriority");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) {
	_logger->info("Called OnDataAvailable");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnLowResource(DWORD dwReserved) {
	_logger->info("Called OnLowResource");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnObjectAvailable(REFIID riid, IUnknown* punk) {
	_logger->info("Called OnObjectAvailable");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnProgress(unsigned long ulProgress, unsigned long ulProgressMax,
	unsigned long ulStatusCode, LPCWSTR szStatusText) {
	_logger->info("Called OnProgress: ulProgress {}, ulProgressMax: {}, ulStatusCode: {}, szStatusText: {}", ulProgress, ulProgressMax, ulStatusCode, szStatusText);

	progress = ulProgress;
	max_dl = ulProgressMax;
	status_code = ulStatusCode;
	dlg->send_message(MSG_UPDATE_DL_PROGRESS);

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnStartBinding(DWORD dwReserved, IBinding* pib) {
	_logger->info("Called OnStartBinding");

	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::OnStopBinding(HRESULT hresult, LPCWSTR szError) {
	_logger->info("Called OnStopBinding");
	finished = true;
	dlg->send_message(MSG_UPDATE_DL_PROGRESS);
	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

HRESULT http_requests::Callback::QueryInterface(REFIID riid, void** ppvObject) {
	if (cancel_download)
		return E_ABORT;
	else
		return S_OK;
}

