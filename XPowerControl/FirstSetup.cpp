#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "FirstSetupDlg2.h"
#include "FirstSetupDlg3.h"
#include "FirstSetupDlg4.h"
#include "FirstSetup.h"
#include "afxdialogex.h"
#include "wstring_transform.h"
#include "TokenRetriever.h"
#include <memory>
#include <iostream>
#include <Windows.h>
#include <string>
#include <cstring>      ///< memset
#include <errno.h>      ///< errno
#include <winsock2.h>   ///< socket
#include <WS2tcpip.h>
#include <boost/format.hpp>
#include "bit7zlibrary.hpp"
#include "bitextractor.hpp"
#include "bitformat.hpp"
#include "bitexception.hpp"
#include "bitmemextractor.hpp"
#include <io.h>
#include <vector>
#include <map>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
#include <algorithm>
#include <fstream>
#include <numeric>
#include <assert.h>
#include <shlobj.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "urlmon.lib")

using namespace std;

void FirstSetup::setup_mitm()
{

	// retrieve home path for user running the program
	wchar_t home_path[MAX_PATH];
	HANDLE current_user;
	OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &current_user);
	SHGetFolderPath(NULL, CSIDL_PROFILE, current_user, SHGFP_TYPE_CURRENT, home_path);
	wstring cert_path = wstring(home_path) + L"\\.mitmproxy\\";

	CreateDirectory(cert_path.c_str(), NULL);
	// delete existing certificates
	vector<wstring> cert_names = { L"mitmproxy-ca.p12", L"mitmproxy-ca.pem", L"mitmproxy-ca-cert.cer",
		L"mitmproxy-ca-cert.p12", L"mitmproxy-ca-cert.pem", L"mitmproxy-dhparam.pem" };
	for (wstring name : cert_names) {
		DeleteFile((cert_path + name).c_str());
	}

	try {
		bit7z::Bit7zLibrary lib{ L"7zxa.dll" };
		bit7z::BitExtractor extractor{ lib, bit7z::BitFormat::SevenZip };
		extractor.extract(L"mitmproxy_certs.7z", cert_path);
	}
	catch (const bit7z::BitException & ex) {
		AfxMessageBox(transform::s2ws(ex.what()).c_str());
		AfxThrowUserException();
	}
}

HANDLE FirstSetup::job_to_process_handle(HANDLE job_handle) {
	// get process handle from job object
	JOBOBJECT_BASIC_PROCESS_ID_LIST id_list;
	DWORD return_size;
	QueryInformationJobObject(job_handle, JobObjectBasicProcessIdList, &id_list, sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST), &return_size);
	DWORD process_id = id_list.ProcessIdList[0];
	return OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, true, process_id);
}

void FirstSetup::setup_bs() {
	setup_bs(optional<CWnd*>());
}

BOOL CALLBACK kill_proxywin(HWND hwnd, LPARAM lParam) {
	wchar_t title[1024];
	GetWindowText(hwnd, title, 1024);
	//wcout << title << L"\n";

	bool visibility = IsWindowVisible(hwnd);
	HWND parent = GetParent(hwnd);

	wchar_t parent_title[1024];
	if (parent != NULL)
		GetWindowText(parent, parent_title, 1024);

	WINDOWINFO window_info;
	window_info.cbSize = sizeof(WINDOWINFO);

	GetWindowInfo(hwnd, &window_info);
	auto width = window_info.rcWindow.right - window_info.rcWindow.left;
	auto height = window_info.rcWindow.bottom - window_info.rcWindow.top;

	if (wstring(title) == L"BlueStacks" && to_wstring(window_info.dwExStyle) == L"3758164225" && visibility == true) {

		wcout << L"Window Title: " << title << L"\n";
		wcout << L"Visibility: " << to_wstring(visibility) << L"\n";
		wcout << L"Has Parent: " << to_wstring(parent != NULL) << L"\n";
		if (parent != NULL)
			wcout << L"Parent Title: " << parent_title << L"\n";


		wcout << L"Window rect: " << height << L"x" << width << L"\n";
		wcout << L"Extended window styles:" << to_wstring(window_info.dwExStyle) << L"\n\n";
		
		// we send a WM_CLOSE message to the program
		SendMessage(hwnd, WM_CLOSE, NULL, NULL);
	}
	return true;
}

void FirstSetup::setup_bs(optional<CWnd*> feedback_win) {
	CRegKey registry;
	ULONG sz_datadir = MAX_PATH;
	ULONG sz_installdir = MAX_PATH;
	CString cstr_datadir = CString();
	CString cstr_installdir = CString();

	if (feedback_win) {
		(*feedback_win)->SetWindowTextW(L"Moving emulator data...");
	}

	// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\BlueStacks has info we need in DataDir and InstallDir
	registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
	registry.QueryStringValue(L"DataDir", cstr_datadir.GetBuffer(sz_datadir), &sz_datadir);
	registry.QueryStringValue(L"InstallDir", cstr_installdir.GetBuffer(sz_installdir), &sz_installdir);

	wstring datadir = wstring(cstr_datadir);
	wstring installdir = wstring(cstr_installdir);

	// move Root.vdi
	wstring vdi_path = datadir + L"Android\\Root.vdi";
	DeleteFile(vdi_path.c_str());
	MoveFile(L"Root.vdi", vdi_path.c_str());

	if (feedback_win)
		(*feedback_win)->SetWindowTextW(L"Adjusting android proxy settings...");

	// launch BlueStacks
	HANDLE bs_handle = TokenRetriever::run_command(datadir + L"..\\Client\\Bluestacks.exe");

	// keep running proxy setup until it returns success code
	DWORD exit_code;
	DWORD gui_count = 0;
	HANDLE process_handle;
	HANDLE exec_handle;
	vector<HANDLE> proxy_handles;
	// run code to set a proxy - this only works when the BlueStacks emulator has loaded, so after this loop ends, we know that the emulator is running
	do {
		exec_handle = TokenRetriever::run_command(installdir + L"HD-ConfigHttpProxy.exe set " + transform::s2ws(get_local_ip()) + L" 8080");
		proxy_handles.push_back(exec_handle);
		process_handle = job_to_process_handle(exec_handle);
		WaitForSingleObject(process_handle, 7000); //TODO: This would work way better if we regularly checked if the window has opened
		

		// if GetGuiResources returns a non-zero value, a UI window has been opened (which means the proxy change succeeded)
		gui_count = GetGuiResources(process_handle, GR_USEROBJECTS);
		cout << "Got GUI count: " << to_string(gui_count) << "\n";
		if (!GetExitCodeProcess(process_handle, &exit_code)) {
			cout << "GetExitCodeProcess Error: " << to_string(GetLastError()) << "\n";
			continue;
		}

		cout << "Exit code: " << to_string(exit_code) << "\n";
		if (exit_code == 259 && gui_count > 3) { // this code is active when a pop-up is on the screen, we can force quit from here
			// close all instances of the proxy program we've opened so far
			EnumWindows(kill_proxywin, NULL); // sends WM_CLOSE to the proxy window
			//for_each(proxy_handles.begin(), proxy_handles.end(), [](HANDLE h) {CloseHandle(h); });
			break;
		}

		WaitForSingleObject(process_handle, 5000);
		Sleep(10);
	} while (exit_code != 0);

	if (feedback_win)
		(*feedback_win)->SetWindowTextW(L"Waiting for emulator to automatically shut down...");

	// we run HD-Quit.exe to quit bluestacks
	HANDLE quit_handle = TokenRetriever::run_command(installdir + L"HD-Quit.exe");
	HANDLE bs_process_handle = job_to_process_handle(bs_handle);

	WaitForSingleObject(bs_process_handle, INFINITE);

	//CloseHandle(bs_handle);

	if (feedback_win)
		(*feedback_win)->SetWindowTextW(L"Installing Nintendo Switch Online app...");

	// run code to install NSO app
	exec_handle = TokenRetriever::run_command(installdir + L"HD-ApkHandler.exe nso.apk");
	process_handle = job_to_process_handle(exec_handle);
	WaitForSingleObject(process_handle, INFINITE);
	GetExitCodeProcess(process_handle, &exit_code);
}

void FirstSetup::cleanup()
{
	DeleteFile(L"Root.7z.001");
	DeleteFile(L"Root.7z.002");
	DeleteFile(L"Root.7z.003");
	DeleteFile(L"Root.7z.004");
	DeleteFile(L"Root.7z");
	DeleteFile(L"nso.apk");
	DeleteFile(L"installer_bs.exe");
	DeleteFile(L"installer_mitm.exe");
	DeleteFile(L"Root.vdi");
	DeleteFile(L"mitmproxy_certs.7z");
}


// using script from https://stackoverflow.com/a/49336660
string FirstSetup::get_local_ip() {
	const char* google_dns_server = "8.8.8.8";
	int dns_port = 53;
	int startup_ret;
	int retries = 0;
	do {
		WORD w_version_requested = MAKEWORD(2, 2);
		WSADATA wsa_data;

		startup_ret = WSAStartup(w_version_requested, &wsa_data);
		if (startup_ret != 0)
			cout << "Startup error " << to_string(startup_ret) << "\n";
		
	} while (startup_ret != 0 && ++retries < 2);

	struct sockaddr_in serv;
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

	//Socket could not be created
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Socket error " << to_string(WSAGetLastError()) << std::endl;

	}

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(google_dns_server);
	serv.sin_port = htons(dns_port);

	int err = connect(sock, (const struct sockaddr*) & serv, sizeof(serv));
	if (err < 0)
	{
		std::cout << "Error number: " << errno
			<< ". Error message: " << strerror(errno) << std::endl;
	}

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*) & name, &namelen);

	char buffer[80];
	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
	if (p != NULL)
	{
		std::cout << "Local IP address is: " << buffer << std::endl;
	}
	else
	{
		std::cout << "Error number: " << errno
			<< ". Error message: " << strerror(errno) << std::endl;
	}

	closesocket(sock);
	WSACleanup();
	return buffer;
}

int FirstSetup::confirm_exit(HWND owner)
{
	return (MessageBox(owner, L"This will exit the setup process. Are you sure?", L"Exit Setup", MB_YESNO | MB_ICONINFORMATION) == IDYES);
}
