// FirstSetupDlg2.cpp : implementation file
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "afxdialogex.h"
#include "FirstSetupDlg2.h"
#include "FirstSetupDlg3.h"
#include "FirstSetupDlg4.h"
#include "FirstSetup.h"
#include "wstring_transform.h"
#include <memory>
#include <iostream>
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
#include <iterator>
#include <functional>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "urlmon.lib")

#define MITM_URL L"https://snapshots.mitmproxy.org/5.0.1/mitmproxy-5.0.1-windows-installer.exe"
#define BS_URL L"https://github.com/snowpoke/XPowerControl-public/raw/master/BlueStacksInstaller_4.170.10.1001_native_0c445ef78c8d14e4e7207acac4a08169.exe"
#define VMACHINE1_URLSTEM L"https://github.com/snowpoke/XPowerControl-public/raw/master/" /* file name has to be added dynamically */
#define VMACHINE2_URL L"https://github.com/snowpoke/XPowerControl-public/raw/master/vmachine.7z.002"
#define VMACHINE_PART1 L"https://github.com/snowpoke/XPowerControl-public/raw/master/Root.7z.001"
#define VMACHINE_PART2 L"https://github.com/snowpoke/XPowerControl-public/raw/master/Root.7z.002"
#define VMACHINE_PART3 L"https://github.com/snowpoke/XPowerControl-public/raw/master/Root.7z.003"
#define VMACHINE_PART4 L"https://github.com/snowpoke/XPowerControl-public/raw/master/Root.7z.004"


using namespace std;
typedef unsigned char byte_t;

// FirstSetupDlg2 dialog
IMPLEMENT_DYNAMIC(FirstSetupDlg2, CDialogEx)


// FirstSetupDlg4 message handlers



void write_binary_buffer(wstring path_t, vector<byte_t> buffer_t) {
	ofstream out_file;
	// we have to convert vector<byte_t> to vector<char>
	DeleteFile(path_t.c_str());
	out_file.open(path_t, ios::out | ios::binary);
	const int STEPSIZE = min(20000, buffer_t.size());
	for (auto it = buffer_t.begin(); ; advance(it, STEPSIZE)) {
		vector<char> buffer_writeable;
		bool is_final_step = (distance(it, buffer_t.end()) <= STEPSIZE);
		buffer_writeable =  is_final_step ?
			 vector<char>(it, buffer_t.end()) : vector<char>(it, it + STEPSIZE);

		out_file.write(buffer_writeable.data(), buffer_writeable.size());
		if (is_final_step)
			break;
	}

	out_file.close();
}

UINT inject_ip_thread(LPVOID pParam) {
	FirstSetupDlg2* dlg = (FirstSetupDlg2*)pParam;

	dlg->progress_dl.SetPos(0);
	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Unpacking android system image...");
	// unpack vmachine.7z and store it in vmachine_buffer
	map<wstring,vector<byte_t>> vmachine_buffer_full;
	try {
		bit7z::Bit7zLibrary lib{ L"7zxa.dll" };
		bit7z::BitExtractor extractor{ lib, bit7z::BitFormat::SevenZip };
		extractor.extract(L"vmachine.7z", vmachine_buffer_full);
	}
	catch (const bit7z::BitException & ex) {
		AfxMessageBox(transform::s2ws(ex.what()).c_str());
		AfxThrowUserException();
	}

	auto vmachine_buffer = vmachine_buffer_full[L"BignoxVMS\\nox\\nox-disk2.vmdk"];

	dlg->progress_dl.SetPos(33);
	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Injecting IP address into image...");
	// get ip
	string local_ip = FirstSetup::get_local_ip();

	// inject ip into vmachine
	int add_digits = local_ip.size() - 7; // number of digits added to search string
	string replace_string_raw = "inshere";
	if (add_digits > 7) { // if the ip has more than 7 characters, add filler 123...
		vector<int> digits(add_digits);
		iota(digits.begin(), digits.end(), 8);
		string temp = "";
		for_each(digits.begin(), digits.end(), [&](int& n) {temp += to_string(n); });
		replace_string_raw += temp;
	}
	vector<byte_t> replace_string(replace_string_raw.begin(), replace_string_raw.end());
	// store iterator for ip address into ip_iterator
	vector<byte_t>::iterator ip_iterator = search(vmachine_buffer.begin(), vmachine_buffer.end(), replace_string.begin(), replace_string.end());
	// write ip at ip_iterator
	vector<byte_t>::iterator copy_end = copy(local_ip.begin(), local_ip.end(), ip_iterator);

	// obtain nox installation path, if nox is already installed
	wstring bin_path;
	dlg->progress_dl.SetPos(66);
	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Saving new android system image...");
	if (!dlg->needs_bs) {
		CRegKey registry;
		CString nox_uinst_path = CString();
		ULONG sz_path = 255;
		auto nox_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Nox", KEY_READ);
		if (nox_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
			registry.QueryStringValue(L"UninstallString", nox_uinst_path.GetBuffer(sz_path), &sz_path);
		}
		else {
			AfxThrowUserException();
		}

		// nox_path now contains the uninstaller file - we navigate from there to the intended path for the image
		wstring wstr_nox_uinst_path(nox_uinst_path);
		int bslash = wstr_nox_uinst_path.rfind(L'\\');
		wstr_nox_uinst_path.erase(bslash);

		// write vmachine into nox folder
		bin_path = wstr_nox_uinst_path + L"\\BignoxVMS\\nox\\nox-disk2.vmdk";

		// copy all files that aren't the system image
		for_each(vmachine_buffer_full.begin(), vmachine_buffer_full.end(), [wstr_nox_uinst_path](pair<wstring, vector<byte_t>> s) {
			if (s.first.find(L"nox-disk2.vmdk") == wstring::npos)
				write_binary_buffer(wstr_nox_uinst_path + L"\\" + s.first, s.second);
		});

	}
	else {
		bin_path = L"nox-disk2.vmdk";
	}

	write_binary_buffer(bin_path, vmachine_buffer);
	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Preparation complete!");
	dlg->progress_dl.SetPos(100);
	return 0;
}


FirstSetupDlg2::FirstSetupDlg2(bool needs_bs_t, bool needs_mitm_t, CWnd* pParent /*=nullptr*/)
	: CustomDialogBase(IDD_FIRSTSETUP2, pParent)
{
	needs_bs = needs_bs_t;
	needs_mitm = needs_mitm_t;
	register_message_mapping(MSG_UPDATE_DL_PROGRESS, (std::function<void(void*)>) on_update_dl_progress_wrapper);
	_logger = logging::get_logger(DEFAULT_LOG);
}

FirstSetupDlg2::~FirstSetupDlg2()
{
}

void FirstSetupDlg2::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_DL, progress_dl);
}

/* legacy message listener
UINT FirstSetupDlg2::message_listener(LPVOID pParam) {
	FirstSetupDlg2* dlg = (FirstSetupDlg2*)pParam;
	while (dlg->message != MSG_STOP_LISTENER) {
		switch (dlg->message) {
		case MSG_UPDATE_DL_PROGRESS:
			dlg->on_update_dl_progress();
			dlg->message = MSG_NONE;
			break;

		}
		Sleep(1);
	}

	return 0;
}*/

BEGIN_MESSAGE_MAP(FirstSetupDlg2, CDialogEx)
	ON_BN_CLICKED(IDOK, &FirstSetupDlg2::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &FirstSetupDlg2::OnBnClickedCancel)
END_MESSAGE_MAP()

// FirstSetupDlg2 message handlers
BOOL FirstSetupDlg2::OnInitDialog() {
	CDialogEx::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	string local_ip = FirstSetup::get_local_ip();
	wstring savepath = L"./";

	mitm_callback = http_requests::Callback(this);
	bs_callback = http_requests::Callback(this);
	nso_callback = http_requests::Callback(this);
	vmachine_callback = { http_requests::Callback(this), http_requests::Callback(this), http_requests::Callback(this), http_requests::Callback(this) };

	winthread_message_listener = AfxBeginThread(message_listener, this);

	// load mitmproxy if neccessary
	if (needs_mitm) {
		DeleteFile(L"installer_mitmproxy.exe");
		AfxBeginThread(http_requests::thread_download_file, new http_requests::DownloadParam{ MITM_URL, L"installer_mitmproxy.exe", &mitm_callback });
	}
	else {
		GetDlgItem(IDC_STATIC_MITM_PROGRESS)->SetWindowText(L"Installed");
	}

	// load nox if neccessary
	if (needs_bs) {
		DeleteFile(L"installer_bs.exe");
		AfxBeginThread(http_requests::thread_download_file, new http_requests::DownloadParam{ BS_URL, L"installer_bs.exe", &bs_callback });
	}
	else {
		GetDlgItem(IDC_STATIC_NOX_PROGRESS)->SetWindowText(L"Installed");
	}
	// always download virtual machine and nso app
	DeleteFile(L"Root.7z.001");
	DeleteFile(L"Root.7z.002");
	DeleteFile(L"Root.7z.003");
	DeleteFile(L"Root.7z.004");
	vmachine_dl_param[0] = http_requests::DownloadParam{ VMACHINE_PART1, L"Root.7z.001", &vmachine_callback[0] };
	vmachine_dl_param[1] = http_requests::DownloadParam{ VMACHINE_PART2, L"Root.7z.002", &vmachine_callback[1] };
	vmachine_dl_param[2] = http_requests::DownloadParam{ VMACHINE_PART3, L"Root.7z.003", &vmachine_callback[2] };
	vmachine_dl_param[3] = http_requests::DownloadParam{ VMACHINE_PART4, L"Root.7z.004", &vmachine_callback[3] };
	AfxBeginThread(http_requests::thread_download_file, &vmachine_dl_param[0]);
	AfxBeginThread(http_requests::thread_download_file, &vmachine_dl_param[1]);
	AfxBeginThread(http_requests::thread_download_file, &vmachine_dl_param[2]);
	AfxBeginThread(http_requests::thread_download_file, &vmachine_dl_param[3]);

	DeleteFile(L"nso.apk");
	AfxBeginThread(http_requests::thread_download_file,
		new http_requests::DownloadParam{ transform::s2ws(http_requests::get_global_info("nso_dl_link")), L"nso.apk", &nso_callback });

	return true;
}

unsigned long get_file_size(ifstream& file) {
	file.seekg(0, file.end); // set position to end of stream
	auto length = file.tellg(); // get position
	file.seekg(0, file.beg); // set position back to beginning of stream

	return length;
}

void FirstSetupDlg2::on_update_dl_progress_wrapper(void* pParam) { // this lets us use a static function in the message handling system without making on_update_dl_progress too messy
	FirstSetupDlg2* dlg = (FirstSetupDlg2*)pParam;
	dlg->on_update_dl_progress();
}


void FirstSetupDlg2::on_update_dl_progress() {
	array<long, 4> vmachine_callback_progress_arr, vmachine_callback_max_arr;
	long vmachine_callback_progress = std::accumulate(vmachine_callback.begin(),
		vmachine_callback.end(),
		0,
		[](long v, http_requests::Callback& c) {return v + c.progress; });
	long vmachine_callback_max = std::accumulate(vmachine_callback.begin(),
		vmachine_callback.end(),
		0,
		[](long v, http_requests::Callback& c) {return v + c.max_dl; });
	long dl_progress_sum = mitm_callback.progress + bs_callback.progress + vmachine_callback_progress + nso_callback.progress;
	long dl_max_sum = mitm_callback.max_dl + bs_callback.max_dl + vmachine_callback_max + nso_callback.max_dl;
	bool dl_finished = true;
	// status bar
	if (dl_max_sum > 0) {
		int pos = floor(static_cast<long double>(dl_progress_sum) * 100.0 / static_cast<long double>(dl_max_sum));
		progress_dl.SetPos(pos);
	}

	// status text
	boost::format mb_format("%.1fMB");
	string dl_progress_mb = (mb_format % (static_cast<long double>(dl_progress_sum) / 1000000.0)).str();
	string dl_max_mb = (mb_format % (static_cast<long double>(dl_max_sum) / 1000000.0)).str();
	wstring status_message = L"Loaded " + transform::s2ws(dl_progress_mb) + L" out of " + transform::s2ws(dl_max_mb) + L".";
	GetDlgItem(IDC_STATIC_DL)->SetWindowText(status_message.c_str());

	// status per download
	if (needs_bs) {
		string bs_progress_mb = (mb_format % (static_cast<long double>(bs_callback.progress) / 1000000.0)).str();
		string bs_max_mb = (mb_format % (static_cast<long double>(bs_callback.max_dl) / 1000000.0)).str();
		status_message = transform::s2ws(bs_progress_mb) + L" / " + transform::s2ws(bs_max_mb);
		GetDlgItem(IDC_STATIC_NOX_PROGRESS)->SetWindowText(status_message.c_str());
		dl_finished = dl_finished && bs_callback.finished;
	}

	if (needs_mitm) {
		string mitm_progress_mb = (mb_format % (static_cast<long double>(mitm_callback.progress) / 1000000.0)).str();
		string mitm_max_mb = (mb_format % (static_cast<long double>(mitm_callback.max_dl) / 1000000.0)).str();
		status_message = transform::s2ws(mitm_progress_mb) + L" / " + transform::s2ws(mitm_max_mb);
		GetDlgItem(IDC_STATIC_MITM_PROGRESS)->SetWindowText(status_message.c_str());
		dl_finished = dl_finished && mitm_callback.finished;
	}

	string vmachine_progress_mb = (mb_format % (static_cast<long double>(vmachine_callback_progress) / 1000000.0)).str();
	string vmachine_max_mb = (mb_format % (static_cast<long double>(vmachine_callback_max) / 1000000.0)).str();
	status_message = transform::s2ws(vmachine_progress_mb) + L" / " + transform::s2ws(vmachine_max_mb);
	GetDlgItem(IDC_STATIC_VMACHINE_PROGRESS)->SetWindowText(status_message.c_str());
	dl_finished = std::accumulate(vmachine_callback.begin(),
		vmachine_callback.end(),
		dl_finished,
		[](bool a, http_requests::Callback& b) {return a && b.finished; }); // dl_finished = dl_finished && vmachine_callback[0].finished && ...

	string nso_progress_mb = (mb_format % (static_cast<long double>(nso_callback.progress) / 1000000.0)).str();
	string nso_max_mb = (mb_format % (static_cast<long double>(nso_callback.max_dl) / 1000000.0)).str();
	status_message = transform::s2ws(nso_progress_mb) + L" / " + transform::s2ws(nso_max_mb);
	GetDlgItem(IDC_STATIC_NSO_PROGRESS)->SetWindowText(status_message.c_str());
	dl_finished = dl_finished && nso_callback.finished;

	if (dl_finished) {
		// merge vmachine parts
		_logger->info("Downloads have finished. Restoring virtual machine archive.");
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Restoring virtual machine archive...");
		array<ifstream, 4> vmachine_parts = { ifstream("Root.7z.001", ios::in | ios::binary),
			ifstream("Root.7z.002", ios::in | ios::binary),
			ifstream("Root.7z.003", ios::in | ios::binary),
			ifstream("Root.7z.004", ios::in | ios::binary)};

		unsigned long machine_size = std::accumulate(vmachine_parts.begin(), vmachine_parts.end(), 0,
			[](unsigned long a, ifstream& b) {return a + get_file_size(b); });
		vector<unsigned char> vmachine;
		vmachine.reserve(machine_size);

		// since we insert at the beginning iterator, we have to iterate in reverse
		std::reverse(vmachine_parts.begin(), vmachine_parts.end());
		for (ifstream& stream : vmachine_parts) {
			auto size = get_file_size(stream);
			vector<char> this_part(size);
			stream.read((char*) &this_part[0], size);
			vmachine.insert(vmachine.begin(), this_part.begin(), this_part.end());
		}
		std::reverse(vmachine_parts.begin(), vmachine_parts.end());

		progress_dl.SetPos(0);
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Extracting virtual machine archive...");
		_logger->info("Extracting virtual machine archive");
		try {
			bit7z::Bit7zLibrary lib{ L"7zxa.dll" };
			bit7z::BitMemExtractor extractor{ lib, bit7z::BitFormat::SevenZip };
			extractor.extract(vmachine); // this creates files root.vdi and certificate.7z in current folder
		}
		catch (const bit7z::BitException & ex) {
			AfxMessageBox(transform::s2ws(ex.what()).c_str());
			AfxThrowUserException();
		}
		
		progress_dl.SetPos(66);
		if (!needs_mitm) { // if MITM is already installed, we manipulate the certificates
			GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Adjusting mitmproxy installation...");
			FirstSetup::setup_mitm();
		}

		if (!needs_bs) { // if bluestacks is already installed, prepare the bluestacks installation for use with the program
			GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Adjusting BlueStacks installation (this might take a bit)...");
			FirstSetup::setup_bs(GetDlgItem(IDC_STATIC_STATUS));
		}

		progress_dl.SetPos(100);
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Done!");
		GetDlgItem(IDOK)->EnableWindow(true);
	}
}


void FirstSetupDlg2::OnBnClickedOk()
{
	// if a program that needs to be installed was downloaded, we open the installer dialog
	send_message(MSG_STOP_LISTENER);
	WaitForSingleObject(winthread_message_listener->m_hThread, 1000);
	ShowWindow(SW_HIDE);
	if (needs_mitm || needs_bs) {
		optional<string> bs_file;
		optional<string> mitm_file;
		if (needs_bs)
			bs_file = "installer_bs.exe";
		if (needs_mitm)
			mitm_file = "installer_mitmproxy.exe";
		FirstSetupDlg3 dlg(bs_file, mitm_file);
		dlg.DoModal();
	}
	else {
		FirstSetupDlg4 dlg;
		dlg.DoModal();
	}
	CDialogEx::OnOK();
}


void FirstSetupDlg2::OnBnClickedCancel()
{
	if (FirstSetup::confirm_exit(this->GetSafeHwnd())) {
		send_message(MSG_STOP_LISTENER);
		WaitForSingleObject(winthread_message_listener->m_hThread, 1000);
		FirstSetup::cleanup();
		CustomDialogBase::OnCancel();
		PostQuitMessage(0);
	}
}
