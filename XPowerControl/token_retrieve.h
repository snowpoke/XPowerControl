#pragma once
#include "stdafx.h"

// DEPRECATED
HANDLE mitm_start();
HANDLE mitm_start_alt();
void kill_mitm(RetrieveTokenDlg* dlg_t);
UINT token_listener(LPVOID pParam);
void log_manually(std::string message);