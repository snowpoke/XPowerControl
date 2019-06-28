#include "stdafx.h"
#include "RetrieveTokenDlg.h"
#include "token_retrieve.h"
#include "nlohmann/json.hpp"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <curl/curl.h>

using namespace std;

HANDLE mitm_start() { // removes previous token and starts mitmdump.exe
	DeleteFile(L"token.txt");
	SHELLEXECUTEINFO exec_info;
	exec_info.cbSize = sizeof(SHELLEXECUTEINFO);
	exec_info.fMask = NULL;
	exec_info.hwnd = NULL;
	exec_info.lpVerb = L"open";
	exec_info.lpFile = L"mitmdump.exe";
	exec_info.lpParameters = L"-s mitm_script.py";
	exec_info.lpDirectory = NULL;
	exec_info.nShow = SW_HIDE;
	exec_info.hInstApp = NULL;
	ShellExecuteEx(&exec_info);

	return exec_info.hProcess;
}


static int writer(char *data, size_t size, size_t nmemb,
	std::string *writerData)
{
	if (writerData == NULL)
		return 0;

	writerData->append(data, size*nmemb);

	return static_cast<int>(size * nmemb);
}


wstring access_token_to_iksm(string access_token_t) {
	CURL *curl1;
	CURL *curl2;
	CURLcode res;
	wstring ret = L"";

	// we obtain the device registration token
	ifstream file;
	file.open("registration_token.txt");
	string line;
	string file_text = "";

	while (getline(file, line)) {
		file_text += line;
	}

	nlohmann::json j = nlohmann::json::parse(file_text);
	string registration_token = j["parameter"]["registrationToken"].get<string>();

	// we obtain the WebServiceToken
	curl1 = curl_easy_init();

	if (curl1) {
		struct curl_slist *chunk = NULL;
		string req_url = "https://api-lp1.znc.srv.nintendo.net/v2/Game/GetWebServiceToken";
		static string buffer;

		chunk = curl_slist_append(chunk, "host: api-lp1.znc.srv.nintendo.net");
		chunk = curl_slist_append(chunk, "content-type: application/json");
		//chunk = curl_slist_append(chunk, ("authorization: Bearer " + access_token_t).c_str());
		chunk = curl_slist_append(chunk, ("Authorization: Bearer " + access_token_t).c_str());


		curl_easy_setopt(curl1, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl1, CURLOPT_URL, req_url.c_str());
		curl_easy_setopt(curl1, CURLOPT_WRITEFUNCTION, writer);
		curl_easy_setopt(curl1, CURLOPT_WRITEDATA, &buffer);

		curl_easy_setopt(curl1, CURLOPT_POST, 1L);

		string data = "{\"parameter\": {\"f\": \"abcd\",\"id\": 5741031244955648,\"registrationToken\": \""
			+ registration_token
			+ "\",\"requestId\": \"abcd\",\"timestamp\": 0 }}";

		curl_easy_setopt(curl1, CURLOPT_POSTFIELDS, data.c_str());

		res = curl_easy_perform(curl1);

		curl_easy_cleanup(curl1);
		curl_slist_free_all(chunk);

		string gamewebtoken = "";
		try {
			nlohmann::json j = nlohmann::json::parse(buffer);
			gamewebtoken = j["result"]["accessToken"].get<string>();
		}
		catch (...) {
			AfxMessageBox(L"Failed retrieving the gamewebtoken.");
			AfxThrowUserException();
		}


		curl2 = curl_easy_init();

		if (curl2) {
			DeleteFile(L"cookies.txt");
			struct curl_slist *chunk = NULL;
			string req_url = "https://app.splatoon2.nintendo.net/?lang=en-GB&na_country=DE&na_lang=en-US";
			string cookie_path = "cookies.txt";

			chunk = curl_slist_append(chunk, "host: app.splatoon2.nintendo.net");
			chunk = curl_slist_append(chunk, ("x-gamewebtoken: " + gamewebtoken).c_str());
			static string buffer;

			curl_easy_setopt(curl2, CURLOPT_COOKIEJAR, cookie_path.c_str());
			curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, chunk);
			curl_easy_setopt(curl2, CURLOPT_URL, req_url.c_str());
			curl_easy_setopt(curl1, CURLOPT_WRITEFUNCTION, writer);
			curl_easy_setopt(curl1, CURLOPT_WRITEDATA, &buffer);

			res = curl_easy_perform(curl2);

			/* always cleanup */
			curl_easy_cleanup(curl2);

			/* free the custom headers */
			curl_slist_free_all(chunk);
			try {
				// we now read the information from the cookie file
				wifstream file;
				file.open(cookie_path);

				wstring line;

				while (getline(file, line)) {
					if (line.size() >= 30 // line must have more characters than length of the token
						&& boost::contains(line, "iksm_session")) { // line must contain iksm_session
						vector<wstring> elems;
						boost::split(elems, line, boost::is_any_of("\t"));
						bool token = false;
						for (wstring elem : elems) { // check for entry iksm_session, then set next entry as return value
							if (token)
								ret = elem;
							token = (boost::contains(elem, "iksm_session"));

						}
					}
				}

				DeleteFile(L"cookies.txt");
			}
			catch (...) {
				AfxMessageBox(L"Failed retrieving the cookie that contrains the token.");
				AfxThrowUserException();
			}
		}
	}
		return ret;
}

UINT token_listener(LPVOID pParam) { // checks if token file exists

	RetrieveTokenDlg* tokenDlg = (RetrieveTokenDlg*)pParam;
	while (true) {
		if (boost::filesystem::exists("authorization.txt")) {
			ifstream file("authorization.txt");
			string authorization_token;
			file >> authorization_token;
			file.close();
			DeleteFile(L"authorization.txt");
			tokenDlg->found_token = access_token_to_iksm(authorization_token);
			tokenDlg->tokenEdit.SetWindowTextW(tokenDlg->found_token.c_str());
			tokenDlg->tokenEdit.EnableWindow(TRUE);
			tokenDlg->ok_btn.EnableWindow(TRUE);
			TerminateProcess(tokenDlg->mitmHandle, 0);
			tokenDlg->mitm_started = false;
			break;
		}
	}

	return 0;
}