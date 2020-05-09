#include "TokenRetriever.h"
#include "stdafx.h"
#include "RetrieveTokenDlg.h"
#include "token_retrieve.h"
#include "nlohmann/json.hpp"
#include "wstring_transform.h"
#include "FirstSetup.h"
#include "ActionRequiredDlg.h"
#include "logging.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <curl/curl.h>
#include <aclapi.h>
#include <tchar.h>
#include <pathcch.h>
#pragma comment(lib, "Pathcch.lib")

using namespace std;

SECURITY_ATTRIBUTES give_all_rights() {

	DWORD dwRes, dwDisposition;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea[2];
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa;
	LONG lRes;
	HKEY hkSub = NULL;

	// Create a well-known SID for the Everyone group.
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = MAXIMUM_ALLOWED;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdminSID))
	{
		_tprintf(_T("AllocateAndInitializeSid Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	ea[1].grfAccessPermissions = MAXIMUM_ALLOWED;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)pAdminSID;

	// Create a new ACL that contains the new ACEs.
	dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes)
	{
		_tprintf(_T("SetEntriesInAcl Error %u\n"), GetLastError());
		goto Cleanup;
	}

	// Initialize a security descriptor.  
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == pSD)
	{
		_tprintf(_T("LocalAlloc Error %u\n"), GetLastError());
		goto Cleanup;
	}

	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION))
	{
		_tprintf(_T("InitializeSecurityDescriptor Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Add the ACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,     // bDaclPresent flag   
		pACL,
		FALSE))   // not a default DACL 
	{
		_tprintf(_T("SetSecurityDescriptorDacl Error %u\n"),
			GetLastError());
		goto Cleanup;
	}

	// Initialize a security attributes structure.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

Cleanup:
	if (pEveryoneSID)
		FreeSid(pEveryoneSID);
	if (pAdminSID)
		FreeSid(pAdminSID);
	if (pACL)
		LocalFree(pACL);
	if (pSD)
		LocalFree(pSD);
	if (hkSub)
		RegCloseKey(hkSub);

	return sa;
}


TokenRetriever::TokenRetriever(std::optional<CWnd*> status_element_t, std::optional<CButton*> ok_button_t, std::wstring mitm_exec_t, std::wstring emu_command_t)
	: status_element(status_element_t), ok_button(ok_button_t), mitm_exec(mitm_exec_t), emu_command(emu_command_t)
{
	_logger = logging::get_logger(DEFAULT_LOG);
}

TokenRetriever::TokenRetriever()
{
	TokenRetriever({}, {}, L"", L"");
}

HANDLE TokenRetriever::run_command(wstring command_t, LPCWSTR current_directory_t /* = NULL */, DWORD creation_flags_t /* = 0 */) {
	
	logging::log_ptr _logger = logging::get_logger(DEFAULT_LOG);
	_logger->info("Running command: \"{}\"", transform::ws2s(command_t));

	// we create an empty job object that will contain the process
	HANDLE job_handle = CreateJobObject(NULL, NULL);

	// we set up the job object so that closing the job causes all child processes to be terminated
	JOBOBJECT_BASIC_LIMIT_INFORMATION kill_flag_info;
	kill_flag_info.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
	job_info.BasicLimitInformation = kill_flag_info;

	SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info));

	// we start the process
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	//SECURITY_ATTRIBUTES sa = give_all_rights(); // transfer all inheritance rights to child processes


	ZeroMemory(&si, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	auto create_success = CreateProcess(NULL,  // lpApplicationName
		&command_t[0], // lpCommandLine
		NULL, // lpProcessAttributes
		NULL, // lpThreadAttributes
		TRUE, // bInheritHandles
		creation_flags_t, // dwCreationFlags
		NULL, // lpEnvironment
		current_directory_t, // lpCurrentDirectory
		&si, // lpStartupInfo
		&pi); // lpProcessInformation

	auto last_error_code = GetLastError();
	_logger->info("CreateProcess returned {}, GetLastError returned {}", to_string(create_success), to_string(last_error_code));

	// we associate the process with the job we created
	AssignProcessToJobObject(job_handle, pi.hProcess);

	return job_handle; // save handle to this object
}


void TokenRetriever::mitm_start() {
	DeleteFile(L"token.txt");

	// delete files that contain results from previous mitmdump processes
	DeleteFile(L"authorization.txt");
	DeleteFile(L"registration_token.txt");
	DeleteFile(L"iksm_cookie.txt");
	// get full path for mitm_script.py
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH); // saves path of current executable in buffer
	PathCchRemoveFileSpec(buffer, MAX_PATH); // remove filename from path
	wstring mitm_script_location = wstring(buffer) + L"\\mitm_script.py";

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

			_logger->info("Found authorization key! Retrieving token... [Auth Key Attempt #{}]", to_string(attempt_num));
			ifstream file;
			file.open("registration_token.txt");
			std::string file_text((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());

			Sleep(50);
		} while (file_text.length() == 0 && attempt_num <= 3);
		_logger->info("File 'registration_token.txt' has been found. Content of the file:\n{}", file_text);
		file_text = remove_multi_response(file_text); // we sometimes get two datasets as a response, we remove all but the first here
		nlohmann::json j = nlohmann::json::parse(file_text);
		registration_token = j["parameter"]["registrationToken"].get<string>();
	}
	catch (const exception & e) {
		AfxMessageBox((L"Failed retrieving the registration token. Please try again or retrieve the token manually. " + transform::s2ws(e.what())).c_str());
		_logger->error("Failed retrieving the registration token. e.what() returned: {}", e.what());
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
				_logger->info("Sending request to https://api-lp1.znc.srv.nintendo.net/v2/Game/GetWebServiceToken.\n \
		Using access token {}\n \
		Using post data {}\n \
		Obtained response {}", access_token_t, data, buffer);
				res = curl_easy_perform(curl1);
			}
			catch (const exception& e) {
				wstring afx_message = L"The program failed to perform the web request. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				AfxMessageBox(afx_message.c_str());
				_logger->error("The program failed to perform the web request. e.what() returned: {}", e.what());
				continue;
			}
			curl_easy_cleanup(curl1);
			curl_slist_free_all(chunk);


			buffer = remove_multi_response(buffer);
			try {
				nlohmann::json j = nlohmann::json::parse(buffer);
				gamewebtoken = j["result"]["accessToken"].get<string>();
			}
			catch (const exception& e) {
				wstring afx_message = L"Failed to read the token from file. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				afx_message += transform::s2ws(e.what());
				AfxMessageBox(afx_message.c_str());
				_logger->error("Failed to read the token from file. (Attempt {}/3) e.what() returned: {}", to_string(attempt_num - 1), e.what());
				continue;
			}

		}
	}

	if (gamewebtoken == "") {
		AfxMessageBox(L"Failed retrieving the gamewebtoken. Please try again or retrieve the token manually.");
		_logger->error("Failed retrieving the gamewebtoken.");
		AfxThrowUserException();
	}

	progress_percent = 90;
	attempt_num = 1;
	while (ret == L"" && attempt_num <= 3) {
		wstring dlg_text = L"Retrieving token... [Authorization Attempt #" + to_wstring(attempt_num) + L"]";
		_logger->info("Retrieving token... [Authorization Attempt #{}]", to_string(attempt_num));
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

			_logger->info("Sending request to https://app.splatoon2.nintendo.net/?lang=en-GB&na_country=DE&na_lang=en-US \n \
							Using x-gamewebtoken {}", gamewebtoken);

			try {
				res = curl_easy_perform(curl2);
			}
			catch (const exception& e) {
				wstring afx_message = L"The program failed to perform the web request. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				afx_message += transform::s2ws(e.what());
				_logger->error("The program failed to perform the web request. (Attempt {}/3) e.what() returned: {}", to_string(attempt_num - 1), e.what());
				AfxMessageBox(afx_message.c_str());
				continue;
			}
			/* always cleanup */
			curl_easy_cleanup(curl2);

			/* free the custom headers */
			curl_slist_free_all(chunk);

			try {
				// for logging purposes, we read and print the entire cookie file
				ifstream temp_file;
				temp_file.open(cookie_path);
				std::string temp_file_text((std::istreambuf_iterator<char>(temp_file)),
					std::istreambuf_iterator<char>());
				temp_file.close();
				_logger->info("Web request returned the following cookie (content of 'cookies.txt'):\n{}", temp_file_text);

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
			catch (const exception& e) {
				wstring afx_message = L"Failed retrieving token information. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
				_logger->error("Failed retrieving token information (Attempt {}/3). e.what() returned {}", to_string(attempt_num - 1), e.what());
				AfxMessageBox(afx_message.c_str());
				continue;
			}
		}
		else {
			wstring afx_message = L"Could not open web connection object. (Attempt " + to_wstring(attempt_num - 1) + L"/3)";
			_logger->error("Could not open web connection object. (Attempt {}/3)", to_string(attempt_num - 1));
			AfxMessageBox(afx_message.c_str());
			continue;
		}
	}

	if (ret == L"") {
		AfxMessageBox(L"The program failed to obtain the authorization token. Please try again or retrieve the token manually.");
		_logger->error("The program failed to obtain the authorization token.");
		AfxThrowUserException();
	}

	_logger->info("access_token_to_iksm returns the following access token: {}", transform::ws2s(ret));

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
			token_retriever->_logger->info("File 'authorization.txt' has been found. Content of the file:\n{}", authorization_token);
			if (authorization_token == "")
				continue;
			DeleteFile(L"authorization.txt");
			ActionRequiredDlg("media/splatoon2_option.png",
				L"Please select the \"Splatoon 2\" option.");
			while (!token_retriever->kill_token_listener) {
				if (boost::filesystem::exists("iksm_cookie.txt")) {
					// the text file describes the cookie that contains the iksm_session parameter
					ifstream file("iksm_cookie.txt");
					string iksm_cookie = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
					file.close();
					token_retriever->_logger->info("File 'iksm_cookie.txt' has been found. Content of the file:\n{}", iksm_cookie);

					if (iksm_cookie == "") // if the file is read as empty, retry
						continue;

					const string SESSION_KEY = "iksm_session=";
					if (iksm_cookie.find(SESSION_KEY) == string::npos) { // check if the term was even found at all
						token_retriever->_logger->info("The key for the iksm session (\"{}\") parameter could not be found within the file.", SESSION_KEY);
						AfxMessageBox(L"The retrieved information is invalid. Please refer to the logs or contact the developer for help.");
						AfxThrowUserException();
					}

					size_t iksm_position = iksm_cookie.find(SESSION_KEY) + SESSION_KEY.length(); // points to the first character after SESSION_KEY
					token_retriever->iksm_token = iksm_cookie.substr(iksm_position, 40);
					token_retriever->_logger->info("Retrieved token: {}", *(token_retriever->iksm_token));
					if (token_retriever->iksm_token->length() < 40 // token needs to have 40 characters
						|| any_of(token_retriever->iksm_token->begin(), token_retriever->iksm_token->end(), [](char c) {return !isalnum(c); })) { // token only contains alphanumerical characters
						token_retriever->_logger->warn("Retrieved token seems to have an invalid shape.");
						AfxMessageBox(L"The retrieved token seems malformed and might not work as intended.");
					}

					DeleteFile(L"iksm_cookie.txt");
					break;
				}
			}
			if (token_retriever->kill_token_listener)
				break;
			//token_retriever->iksm_token = transform::ws2s(token_retriever->access_token_to_iksm(authorization_token));
			Sleep(1000); //TODO: We only actually need to wait here during the first setup (and maybe not at all)
			CloseHandle(token_retriever->mitm_handle);
			token_retriever->_logger->info("Closed MITM handle.");
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

			DWORD ret_code = WaitForSingleObject(bs_process_handle, INFINITE);
			token_retriever->_logger->info("BlueStacks process has been finished. WaitForSingleObject returned: {}", to_string(ret_code));

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