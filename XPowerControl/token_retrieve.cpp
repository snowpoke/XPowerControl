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
	exec_info.nShow = SW_SHOW;
	exec_info.hInstApp = NULL;
	ShellExecuteEx(&exec_info);

	return exec_info.hProcess;
}

HANDLE mitm_start_alt() {
	DeleteFile(L"token.txt");

	// we create an empty job object that will contain the process
	HANDLE job_handle = CreateJobObject(NULL, NULL);

	// we set up the job object so that closing the job causes all child processes to be terminated
	JOBOBJECT_BASIC_LIMIT_INFORMATION kill_flag_info;
	kill_flag_info.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
	job_info.BasicLimitInformation = kill_flag_info;

	SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info));

	// delete files that contain results from previous mitmdump processes
	DeleteFile(L"authorization.txt");
	DeleteFile(L"registration_token.txt");

	// we start mitmdump
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(L"mitmdump.exe",
		L"mitmdump.exe -s mitm_script.py",
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi);

	// we associate the mitmdump with the job we created
	AssignProcessToJobObject(job_handle, pi.hProcess);

	return job_handle;
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

	// wait until registration token file is created
	while (!boost::filesystem::exists("registration_token.txt")) {

	}
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
		
		log_manually("Sending request to https://api-lp1.znc.srv.nintendo.net/v2/Game/GetWebServiceToken.\n \
		Using access token " + access_token_t + "\n \
		Using post data " + data + "\n \
		Obtained response " + buffer + "\n\n\n");

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

			log_manually("Sending request to https://app.splatoon2.nintendo.net/?lang=en-GB&na_country=DE&na_lang=en-US \n \
							Using x-gamewebtoken " + gamewebtoken + "\n\n\n");
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

void kill_mitm(RetrieveTokenDlg* dlg_t) {
	CloseHandle(dlg_t->mitm_handle); // we set up the job so that this kills all child processes
 	dlg_t->mitm_started = false;
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
			tokenDlg->tokenEdit.SetWindowTextW(L"Found authorization key! Retrieving token...");
			tokenDlg->found_token = access_token_to_iksm(authorization_token);
			kill_mitm(tokenDlg);
			tokenDlg->tokenEdit.SetWindowTextW(tokenDlg->found_token.c_str());
			tokenDlg->tokenEdit.EnableWindow(TRUE);
			tokenDlg->ok_btn.EnableWindow(TRUE);
			
			break;
		}
	}

	return 0;
}

void log_manually(string message) {
	ofstream file;
	file.open("log.txt", ios_base::app);
	file << message;
}