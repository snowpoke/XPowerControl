// FirstSetupDlg4.cpp : implementation file
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "afxdialogex.h"
#include "FirstSetupDlg4.h"
#include "FirstSetup.h"
#include "RetrievalProgressDlg.h"
#include "bit7zlibrary.hpp"
#include "bitextractor.hpp"
#include "bitformat.hpp"
#include "bitexception.hpp"
#include "wstring_transform.h"
#include "nlohmann/json.hpp"
#include "http_requests.h"
#include <iostream>
#include <cstring>      ///< memset
#include <errno.h>      ///< errno
#include <winsock2.h>   ///< socket
#include <WS2tcpip.h>
#include <io.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <numeric>
#include <assert.h>

#pragma comment(lib, "ws2_32.lib")


using namespace std;

typedef unsigned char byte_t;
// FirstSetupDlg4 dialog

IMPLEMENT_DYNAMIC(FirstSetupDlg4, CDialogEx)

FirstSetupDlg4::FirstSetupDlg4(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FIRSTSETUP4, pParent)
{
	_logger = logging::get_logger(DEFAULT_LOG);
}

FirstSetupDlg4::~FirstSetupDlg4()
{
}

void FirstSetupDlg4::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(FirstSetupDlg4, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &FirstSetupDlg4::OnBnClickedButton1)
	ON_BN_CLICKED(IDCANCEL, &FirstSetupDlg4::OnBnClickedCancel)
END_MESSAGE_MAP()


BOOL FirstSetupDlg4::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	FirstSetup::cleanup();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	// store nox and mitm commands into settings
	wstring bs_exec;
	CRegKey registry;
	CString bs_install_dir = CString();
	ULONG sz_path = 255;
	auto nox_key_ret = registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
	if (nox_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		registry.QueryStringValue(L"InstallDir", bs_install_dir.GetBuffer(sz_path), &sz_path);
	}
	else {
		AfxThrowUserException();
	}

	// nox_path now contains the uninstaller file - we navigate from there to the intended path for the image
	wstring wstr_nox_uinst_path(bs_install_dir);
	bs_exec = wstr_nox_uinst_path + L"HD-RunApp.exe";
	registry.Close();


	wstring mitm_exec;
	CString mitm_uinst_path = CString();
	sz_path = 255;
	auto mitm_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\mitmproxy 5.0.1", KEY_READ);
	if (mitm_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		registry.QueryStringValue(L"UninstallString", mitm_uinst_path.GetBuffer(sz_path), &sz_path);
	}
	else {
		AfxThrowUserException();
	}

	// mitm_path now contains the uninstaller file - we navigate from there to the intended path for the image
	wstring wstr_mitm_uinst_path(mitm_uinst_path);
	remove(wstr_mitm_uinst_path.begin(), wstr_mitm_uinst_path.end(), L'\"');
	auto bslash = wstr_mitm_uinst_path.rfind(L'\\');
	wstr_mitm_uinst_path.erase(bslash);
	mitm_exec = wstr_mitm_uinst_path + L"\\bin\\mitmdump.exe";
	registry.Close();

	ifstream in_file("settings.txt");
	nlohmann::json j;
	in_file >> j;
	in_file.close();

	j["emu_cmd"] = transform::ws2s(bs_exec + L" -json \"{\\\"app_icon_url\\\":\\\"\\\",\\\"app_name\\\":\\\"Nintendo Switch Online\\\",\\\"app_url\\\":\\\"\\\",\\\"app_pkg\\\":\\\"com.nintendo.znca\\\"}\"");
	j["emu_exec"] = transform::ws2s(bs_exec);
	j["mitm_exec"] = transform::ws2s(mitm_exec);

	ofstream out_file("settings.txt");
	out_file << j;
	out_file.close();

	_logger->info("New settings value for emu_cmd: {}", j["emu_cmd"]);
	_logger->info("New settings value for emu_exec: {}", j["emu_exec"]);
	_logger->info("New settings value for mitm_exec: {}", j["mitm_exec"]);
	

	return 0;
}


void FirstSetupDlg4::OnBnClickedButton1()
{

	_logger->info("Initiating token retrieval.");
	RetrievalProgressDlg dlg;
	dlg.require_login = true;
	dlg.DoModal();

	ifstream in_file("settings.txt");
	nlohmann::json j;
	in_file >> j;
	in_file.close();

	if (j["iksm-session"] != "") {
		string splatnet_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/records",
			j["iksm-session"]);
		if (splatnet_string.find("AUTHENTICATION_ERROR") == std::string::npos) { // if returned data doesn't contain "AUTHENTICATION ERROR"
			_logger->info("User authentification successful. Setting performed_setup flag in settings file.");
			nlohmann::json j = nlohmann::json::parse(splatnet_string);
			string nickname = j["records"]["player"]["nickname"];
			CString welcome = _T("Valid Token! Welcome, ");
			welcome += CString(nickname.c_str());
			welcome += _T("!");
			GetDlgItem(IDC_STATIC_RESPONSE)->SetWindowTextW(welcome);
			GetDlgItem(IDC_STATIC_RESPONSE)->ShowWindow(SW_SHOW);
			GetDlgItem(IDOK)->EnableWindow(true);
			// write to settings that we finished the setup
			ifstream file_in("settings.txt");
			nlohmann::json j_settings;
			file_in >> j_settings;
			file_in.close();
			j_settings["performed_setup"] = true;
			ofstream file_out("settings.txt");
			file_out << j_settings;
			file_out.close();
		}
	}
}


void FirstSetupDlg4::OnBnClickedCancel()
{
	if (FirstSetup::confirm_exit(this->GetSafeHwnd())) {
		FirstSetup::cleanup();
		CDialogEx::OnCancel();
		PostQuitMessage(0);
	}
}
