#include "TokenRetriever.h"
#include "stdafx.h"
#include "RetrieveTokenDlg.h"
#include "token_retrieve.h"
#include "nlohmann/json.hpp"
#include "wstring_transform.h"
#include "FirstSetup.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <curl/curl.h>

using namespace std;

HANDLE TokenRetriever::run_command(wstring command_t) {

	// we create an empty job object that will contain the process
	HANDLE job_handle = CreateJobObject(NULL, NULL);

	// we set up the job object so that closing the job causes all child processes to be terminated
	JOBOBJECT_BASIC_LIMIT_INFORMATION kill_flag_info;
	kill_flag_info.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
	job_info.BasicLimitInformation = kill_flag_info;

	SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info));

	// we start mitmdump
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(NULL,
		&command_t[0],
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

	return job_handle; // save handle to this object
}


void TokenRetriever::mitm_start() {
	DeleteFile(L"token.txt");

	// delete files that contain results from previous mitmdump processes
	DeleteFile(L"authorization.txt");
	DeleteFile(L"registration_token.txt");

	// get full path for mitm_script.py
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH); // saves path of current executable in buffer
	size_t found = wstring(buffer).find_last_of('\\');
	wstring mitm_script_location = wstring(buffer).substr(0, found + 1) + L"mitm_script.py";

	mitm_handle = run_command(mitm_exec + L" -s \"" + mitm_script_location + L"\"");
}

void TokenRetriever::emu_start() {
	emu_handle = run_command(emu_command);
}



int TokenRetriever::writer(char* data, size_t size, size_t nmemb,
	std::string* writerData)
{
	if (writerData == NULL)
		return 0;

	writerData->append(data, size * nmemb);

	return static_cast<int>(size * nmemb);
}


// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// we sometimes get multiple json datasets in one response, we remove all but the first one
string TokenRetriever::remove_multi_response(string buffer_t) {
	ltrim(buffer_t); // we trim from from the left, just to make sure this algorithm doesn't fail because of a space at the start
	int brackets = 0;
	string out_string = "";
	for (char& c : buffer_t) {
		out_string += c;
		if (c == '{')
			brackets++;
		else if (c == '}')
			brackets--;
		if (brackets == 0) // since the very first character is {, this should only be 0 after the section has been closed
			break;
	}

	return out_string;
}


wstring TokenRetriever::access_token_to_iksm(string access_token_t) {
	CURL* curl1;
	CURL* curl2;
	CURLcode res;
	wstring ret = L"";

	if(status_element)
		(*status_element)->SetWindowTextW(L"Found authorization key! Retrieving token... [Auth Key Attempt #1]");

	progress_percent = 70;

	// we obtain the device registration token

	// wait until registration token file is created
	while (!boost::filesystem::exists("registration_token.txt")) {

	}

	string registration_token = "";
	int attempt_num = 0;
	try {
		string file_text = "";
		do {
			attempt_num++;
			if (status_element)
				(*status_element)->SetWindowTextW((L"Found authorization key! Retrieving token... [Auth Key Attempt #"
					+ to_wstring(attempt_num) + L"1]").c_str());
			ifstream file;
			file.open("registration_token.txt");
			string line;

			while (getline(file, line)) {
				file_text += line;
			}
			Sleep(50);
		} while (file_text.length() == 0 && attempt_num <= 3);
		file_text = remove_multi_response(file_text); // we sometimes get two datasets as a response, we remove all but the first here
		nlohmann::json j = nlohmann::json::parse(file_text);
		registration_token = j["parameter"]["registrationToken"].get<string>();
	}
	catch (const exception & e) {
		AfxMessageBox((L"Failed retrieving the registration token. Please try again or retrieve the token manually. " + transform::s2ws(e.what())).c_str());
		AfxThrowUserException();
	}

	if (status_element)
		(*status_element)->SetWindowTextW(L"Retrieving token... [Registration Token OK]");

	progress_percent = 80;
	// we obtain the WebServiceToken
	string gamewebtoken = "";
	attempt_num = 1;
	while (gamewebtoken == "" && attempt_num <= 3) {
		wstring dlg_text = L"Retrieving token... [Gamewebtoken Attempt #" + to_wstring(attempt_num) + L"]";
		if (status_element)
			(*status_element)->SetWindowTextW(dlg_text.c_str());
		attempt_num++;
		curl1 = curl_easy_init();

		if (curl1) {
			struct curl_slist* chunk = NULL;
			string req_url = "https://api-lp1.znc.srv.nintendo.net/v2/Game/GetWebServiceToken";
			static string buffer;

			chunk = curl_slist_append(chunk, "host: api-lp1.znc.srv.nintendo.net");
			chunk = curl_slist_append(chunk, "content-type: application/json");
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

			try {
				res = curl_easy_perform(curl1);
			}
			catch (...) {
				wstring afx_message = L"The program failed to perform the web request. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				AfxMessageBox(afx_message.c_str());
				continue;
			}
			curl_easy_cleanup(curl1);
			curl_slist_free_all(chunk);

			log_manually("Sending request to https://api-lp1.znc.srv.nintendo.net/v2/Game/GetWebServiceToken.\n \
		Using access token " + access_token_t + "\n \
		Using post data " + data + "\n \
		Obtained response " + buffer + "\n\n\n");
			buffer = remove_multi_response(buffer);
			try {
				nlohmann::json j = nlohmann::json::parse(buffer);
				gamewebtoken = j["result"]["accessToken"].get<string>();
			}
			catch (const exception& e) {
				wstring afx_message = L"Failed to read the token from file. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				afx_message += transform::s2ws(e.what());
				AfxMessageBox(afx_message.c_str());
				continue;
			}

		}
	}

	if (gamewebtoken == "") {
		AfxMessageBox(L"Failed retrieving the gamewebtoken. Please try again or retrieve the token manually.");
		AfxThrowUserException();
	}

	progress_percent = 90;
	attempt_num = 1;
	while (ret == L"" && attempt_num <= 3) {
		wstring dlg_text = L"Retrieving token... [Authorization Attempt #" + to_wstring(attempt_num) + L"]";
		if (status_element)
			(*status_element)->SetWindowTextW(dlg_text.c_str());
		attempt_num++;

		curl2 = curl_easy_init();

		if (curl2) {
			DeleteFile(L"cookies.txt");
			struct curl_slist* chunk = NULL;
			string req_url = "https://app.splatoon2.nintendo.net/?lang=en-GB&na_country=DE&na_lang=en-US";
			string cookie_path = "cookies.txt";

			chunk = curl_slist_append(chunk, "host: app.splatoon2.nintendo.net");
			chunk = curl_slist_append(chunk, ("x-gamewebtoken: " + gamewebtoken).c_str());
			static string buffer;

			curl_easy_setopt(curl2, CURLOPT_COOKIEJAR, cookie_path.c_str());
			curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, chunk);
			curl_easy_setopt(curl2, CURLOPT_URL, req_url.c_str());
			curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, writer);
			curl_easy_setopt(curl2, CURLOPT_WRITEDATA, &buffer);

			log_manually("Sending request to https://app.splatoon2.nintendo.net/?lang=en-GB&na_country=DE&na_lang=en-US \n \
							Using x-gamewebtoken " + gamewebtoken + "\n\n\n");

			try {
				res = curl_easy_perform(curl2);
			}
			catch (...) {
				wstring afx_message = L"The program failed to perform the web request. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				AfxMessageBox(afx_message.c_str());
				continue;
			}
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
				wstring afx_message = L"Failed retrieving token information. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				AfxMessageBox(afx_message.c_str());
				continue;
			}
		}
		else {
			wstring afx_message = L"Could not open web connection object. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
			AfxMessageBox(afx_message.c_str());
			continue;
		}
	}

	if (ret == L"") {
		AfxMessageBox(L"The program failed to obtain the authorization token. Please try again or retrieve the token manually.");
		AfxThrowUserException();
	}

	return ret;
}

UINT TokenRetriever::token_listener(LPVOID pParam) { // checks if token file exists
	// this listener expects MITM and emu to be running, waits for results

	TokenRetriever* token_retriever = (TokenRetriever*)pParam;

	while (!token_retriever->kill_token_listener) {
		if (boost::filesystem::exists("authorization.txt")) {
			ifstream file("authorization.txt");
			string authorization_token;
			file >> authorization_token;
			file.close();
			if (authorization_token == "")
				continue;
			DeleteFile(L"authorization.txt");
			token_retriever->iksm_token = transform::ws2s(token_retriever->access_token_to_iksm(authorization_token));
			Sleep(1000); //TODO: We only actually need to wait here during the first setup (and maybe not at all)
			CloseHandle(token_retriever->mitm_handle);
			//CloseHandle(token_retriever->emu_handle);

			if (token_retriever->status_element)
				token_retriever->status_element.value()->SetWindowTextW(L"Sending close signal...");

			// NOTE: Using the same code here as in bs_setup() - turning this into its own function might be worthwhile
			CRegKey registry;
			ULONG sz_installdir = MAX_PATH;
			CString cstr_installdir = CString();

			// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\BlueStacks has info we need in InstallDir
			registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
			registry.QueryStringValue(L"InstallDir", cstr_installdir.GetBuffer(sz_installdir), &sz_installdir);
			wstring installdir = wstring(cstr_installdir);

			// we run HD-Quit.exe to quit bluestacks
			HANDLE quit_handle = TokenRetriever::run_command(installdir + L"HD-Quit.exe");
			HANDLE bs_process_handle = FirstSetup::job_to_process_handle(token_retriever->emu_handle);

			WaitForSingleObject(bs_process_handle, INFINITE);

			//tokenDlg->tokenEdit.SetWindowTextW(tokenDlg->found_token.c_str());
			//tokenDlg->tokenEdit.EnableWindow(TRUE);
			if (token_retriever->ok_button)
				(*(token_retriever->ok_button))->EnableWindow(TRUE);

			break;
		}
		Sleep(50);
	}

	return 0;
}

void TokenRetriever::log_manually(string message) {
	ofstream file;
	file.open("log.txt", ios_base::app);
	file << message;
}