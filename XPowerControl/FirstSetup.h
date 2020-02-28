#pragma once
#include "resource.h"
#include <afxdialogex.h>
#include <string>
#include <optional>

namespace FirstSetup {
	void setup_mitm();
	void setup_bs();
	void setup_bs(std::optional<CWnd*> feedback_win);
	void cleanup();
	std::string get_local_ip();
	HANDLE job_to_process_handle(HANDLE job_handle);
	int confirm_exit(HWND owner = NULL);
}