#include <optional>
#include <string>
#include "resource.h"
#include <afxdialogex.h>


#pragma once
class TokenRetriever
{
public:
	std::optional<CWnd*> status_element;
	std::optional<CButton*> ok_button;
	HANDLE mitm_handle = CreateJobObject(NULL, NULL);
	HANDLE emu_handle = CreateJobObject(NULL, NULL);
	std::wstring mitm_exec;
	std::wstring emu_command;

	// constructor
	TokenRetriever(std::optional<CWnd*> status_element_t, std::optional<CButton*> ok_button_t, std::wstring mitm_exec_t, std::wstring emu_command_t)
		: status_element(status_element_t), ok_button(ok_button_t), mitm_exec(mitm_exec_t), emu_command(emu_command_t) {}

	TokenRetriever() : status_element({}), ok_button({}), mitm_exec(L""), emu_command(L"") {}

	HANDLE run_command(std::wstring command_t);
	void mitm_start();
	void emu_start();
	std::wstring access_token_to_iksm(std::string access_token_t);
	static UINT token_listener(LPVOID pParam);
	std::optional<std::string> get_iksm_token() { return iksm_token; }
	int get_progress() {return progress_percent;}
	void set_progress(int n) { progress_percent = n; }
private:
	static int writer(char* data, size_t size, size_t nmemb,
		std::string* writerData);
	void log_manually(std::string message);
	std::string remove_multi_response(std::string buffer_t);
	int progress_percent = 0;
	std::optional<std::string> iksm_token;
};

