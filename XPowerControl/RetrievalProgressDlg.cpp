// RetrievalProgressDlg.cpp : implementation file
//

#include "RetrievalProgressDlg.h"
#include "afxdialogex.h"
#include "TokenRetriever.h"
#include "settings.h"
#include "wstring_transform.h"
#include "nlohmann/json.hpp"
#include "ActionRequiredDlg.h"
#include "UpdateNSODlg.h"
#include "logging.h"
#include <string>
#include <filesystem>
using namespace std;

// RetrievalProgressDlg dialog

IMPLEMENT_DYNAMIC(RetrievalProgressDlg, CDialogEx)

RetrievalProgressDlg::RetrievalProgressDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RETRIEVE_PROGRESS, pParent)
{
	_logger = logging::get_logger(DEFAULT_LOG);
}

RetrievalProgressDlg::~RetrievalProgressDlg()
{
}

void RetrievalProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_MAIN, progress);
}


BEGIN_MESSAGE_MAP(RetrievalProgressDlg, CDialogEx)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


UINT progress_listener(LPVOID pParam) {
	RetrievalProgressDlg* dlg = (RetrievalProgressDlg*)pParam;

	// at the start of the launching process, we use the number of lines in process.txt to indicate how far we are
	int launching = true;
	bool dlg_opened = false;
	do {
		// check the number of lines, update accordingly
		if (launching) {
			ifstream file("progress.txt");
			string line;
			int line_num = 0;
			while (getline(file, line))
				line_num++;

			if (dlg->require_login)
				dlg->progress.SetPos(min(line_num * 5, 20)); // 5% per line, maximum 20%
			else
				dlg->progress.SetPos(min(line_num * 5,50)); // 5% per line, maximum 50%

			if (line_num > 0 && dlg->require_login && !dlg_opened) {
				ActionRequiredDlg("media/bluestacks_login_comp.png",
					L"Please close the pop-up and log into the Nintendo Switch Online App, the program will then retrieve your token.");
				dlg_opened = true;
			}
		}


		if (dlg->progress.GetPos() != dlg->retriever.get_progress() && dlg->retriever.get_progress() != 0) {
			dlg->progress.SetPos(dlg->retriever.get_progress());
			launching = false;
		}


		Sleep(10);
	} while (dlg->progress.GetPos() < 100);
	dlg->progress.SetPos(dlg->retriever.get_progress());

	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Token has been retrieved successfully!");
	auto val = WaitForSingleObject(dlg->token_listener_thread->m_hThread, 10000);
	dlg->has_succeeded = true;
	if (!dlg->no_closing_after_success) {
		dlg->SendMessage(WM_CLOSE);
	}
	return 0;
}

UINT iksm_listener(LPVOID pParam) {
	RetrievalProgressDlg* dlg = (RetrievalProgressDlg*)pParam;

	while (true) {
		if (dlg->retriever.get_iksm_token().has_value()) {
			ifstream in_file("settings.txt");
			nlohmann::json j;
			in_file >> j;
			in_file.close();

			j["iksm-session"] = dlg->retriever.get_iksm_token().value();

			ofstream out_file("settings.txt");
			out_file << j;
			out_file.close();

			dlg->retriever.set_progress(100);

			break;
		}
		Sleep(10);
	}
	return 0;
}

// RetrievalProgressDlg message handlers

BOOL RetrievalProgressDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	install_validity_check();

	if (UpdateNSODlg::is_update_required()) {
		UpdateNSODlg nso_update_dlg;
		nso_update_dlg.DoModal();
	}


	// make sure the window is in the foreground, even after helper software is called
	::SetForegroundWindow(this->GetSafeHwnd());
	::SetWindowPos(this->GetSafeHwnd(),
		HWND_TOPMOST,
		0,
		0,
		0,
		0,
		SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

	if (require_login) {
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Launching NSO App...\nPlease log in once the app has loaded.");
	}
	progress.SetRange(0, 100);
	progress.SetPos(0);

	// read mitm path and emulator command from settings
	string mitm_exec = read_from_settings<string>("mitm_exec");
	string emu_cmd = read_from_settings<string>("emu_cmd");


	retriever = TokenRetriever(GetDlgItem(IDC_STATIC_STATUS), {}, transform::s2ws(mitm_exec), transform::s2ws(emu_cmd));

	retriever.mitm_start();
	retriever.emu_start();
	
	// reset progress.txt
	if (filesystem::exists("progress.txt"))
		DeleteFile(L"progress.txt");

	// token_listener waits until mitm and emu have loaded, then loads new iksm token and stores it
	token_listener_thread = AfxBeginThread(retriever.token_listener, &retriever);

	// progress_listener listens to progress_percent on the iksm retriever, updates progress bar accordingly
	AfxBeginThread(progress_listener, this);

	// iksm_listener listens to iksm_token, and saves it to settings if found
	AfxBeginThread(iksm_listener, this);

	return TRUE;
}

void RetrievalProgressDlg::install_validity_check()
{
	// check bs version
	CRegKey registry;
	CString bs_version_str;
	ULONG sz_version = 20;
	auto bs_key_ret = registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
	bs_version_str = CString();
	registry.QueryStringValue(L"ClientVersion", bs_version_str.GetBuffer(sz_version), &sz_version);
	Version bs_version = Version::from_string(transform::ws2s(wstring(bs_version_str)));
	Version bs_min_version = Version::from_int(4, 170, 10, 1001);
	if (bs_version != bs_min_version) {
		_logger->warn("The recommended version for BlueStacks is {}, but the user has installed version {}.",
			transform::ws2s(bs_min_version.to_wstring(4)), transform::ws2s(bs_min_version.to_wstring(4)));
		MessageBox(L"Your BlueStacks installation is not on the recommended version. You might encounter problems during the retrieval process.");
	}
	else {
		_logger->info("The user is using the recommended version of Bluestacks.");
	}


	// check mitm signature files
	// retrieve home path for user running the program
	wchar_t home_path[MAX_PATH];
	HANDLE current_user;
	OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &current_user);
	SHGetFolderPath(NULL, CSIDL_PROFILE, current_user, SHGFP_TYPE_CURRENT, home_path);
	wstring cert_path = wstring(home_path) + L"\\.mitmproxy\\";
	vector<wstring> cert_names = { L"mitmproxy-ca.p12", L"mitmproxy-ca.pem", L"mitmproxy-ca-cert.cer",
		L"mitmproxy-ca-cert.p12", L"mitmproxy-ca-cert.pem", L"mitmproxy-dhparam.pem" };

	// check if the expected certificate files exist
	bool all_certs_found = true;
	for (auto& name : cert_names) {
		if (!filesystem::exists(cert_path + name)) {
			_logger->warn("Certificate file was not found, expected file location: {}", transform::ws2s(cert_path + name));
			all_certs_found = false;
		}
	}


	// check if certificate metadata matches
	typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
		DWORD    dwFileAttributes;
		FILETIME ftCreationTime;
		FILETIME ftLastAccessTime;
		FILETIME ftLastWriteTime;
		DWORD    nFileSizeHigh;
		DWORD    nFileSizeLow;

		bool operator==(const _WIN32_FILE_ATTRIBUTE_DATA& rhs) // warning: this is actually not an exact ==, because some elements are ignored
		{
			return (//ftCreationTime.dwHighDateTime == rhs.ftCreationTime.dwHighDateTime
				//&& ftCreationTime.dwLowDateTime == rhs.ftCreationTime.dwLowDateTime
				ftLastWriteTime.dwHighDateTime == rhs.ftLastWriteTime.dwHighDateTime
				&& ftLastWriteTime.dwLowDateTime == rhs.ftLastWriteTime.dwLowDateTime
				&& nFileSizeLow == rhs.nFileSizeLow);
		}

	} WIN32_FILE_ATTRIBUTE_DATA, * LPWIN32_FILE_ATTRIBUTE_DATA; 
	
	WIN32_FILE_ATTRIBUTE_DATA CERTINFO1{ 0, {1358846752, 30796666}, {0, 0},  {873945763, 30699675}, 0, 2529 },
		CERTINFO2{ 0, {1358896605, 30796666}, {0, 0}, {873809279, 30699675}, 0, 3022 },
		CERTINFO3{ 0, {1358707119, 30796666}, {0, 0}, {873848294, 30699675}, 0, 1318 },
		CERTINFO4{ 0, {1358796874, 30796666}, {0, 0}, {873887318, 30699675}, 0, 1140 },
		CERTINFO5{ 0, {1358826796, 30796666}, {0, 0}, {873838540, 30699675}, 0, 1318 },
		CERTINFO6{ 0, {1358946481, 30796666}, {0, 0}, {873984849, 30699675}, 0, 770 };

	map<wstring, WIN32_FILE_ATTRIBUTE_DATA> cert_name_to_info{ {L"mitmproxy-ca.p12", CERTINFO1}, {L"mitmproxy-ca.pem", CERTINFO2},
	{L"mitmproxy-ca-cert.cer", CERTINFO3}, {L"mitmproxy-ca-cert.p12", CERTINFO4},
	{L"mitmproxy-ca-cert.pem", CERTINFO5}, {L"mitmproxy-dhparam.pem", CERTINFO6} };

	WIN32_FILE_ATTRIBUTE_DATA file_info;
	bool cert_metadata_matches = true;
	for (auto& name : cert_names) {
		wstring temp_path = cert_path + name;
		GetFileAttributesEx(temp_path.c_str(), GetFileExInfoStandard, &file_info);

		_logger->info("Found information about file {}: ftCreationTime: {}/{} ftLastAccessTime: {}/{} ftLastWriteTime: {}/{} nFileSizeHigh: {} nFileSizeLow: {}",
			transform::ws2s(temp_path),
			file_info.ftCreationTime.dwLowDateTime, file_info.ftCreationTime.dwHighDateTime,
			file_info.ftLastAccessTime.dwLowDateTime, file_info.ftLastAccessTime.dwHighDateTime,
			file_info.ftLastWriteTime.dwLowDateTime, file_info.ftLastWriteTime.dwHighDateTime,
			file_info.nFileSizeHigh, file_info.nFileSizeLow);

		if (cert_name_to_info[name] == file_info) {
			_logger->info("Metadata for {} matches with expected metadata.", transform::ws2s(temp_path));
		}
		else {
			_logger->warn("Metadata for {} does NOT match with the expected metadata.", transform::ws2s(temp_path));
			cert_metadata_matches = false;
		}
	}


	// check mitm python script
	WIN32_FILE_ATTRIBUTE_DATA SCRIPTINFO{ 0, {1220656956, 30744796}, {0, 0}, {1279978786, 30811548}, 0, 790 };
	bool python_metadata_matches = true;
	GetFileAttributesEx(L"mitm_script.py", GetFileExInfoStandard, &file_info);

	_logger->info("Found information about file mitm_script.py: ftCreationTime: {}/{} ftLastAccessTime: {}/{} ftLastWriteTime: {}/{} nFileSizeHigh: {} nFileSizeLow: {}",
		file_info.ftCreationTime.dwLowDateTime, file_info.ftCreationTime.dwHighDateTime,
		file_info.ftLastAccessTime.dwLowDateTime, file_info.ftLastAccessTime.dwHighDateTime,
		file_info.ftLastWriteTime.dwLowDateTime, file_info.ftLastWriteTime.dwHighDateTime,
		file_info.nFileSizeHigh, file_info.nFileSizeLow);

	if (SCRIPTINFO == file_info) {
		_logger->info("Metadata for mitm_script.py matches with expected metadata.");
	}
	else {
		_logger->warn("Metadata for mitm_script.py does NOT match with the expected metadata.");
		python_metadata_matches = false;
	}

	// check root.vdi
	CString cstr_datadir;
	ULONG sz_datadir = MAX_PATH;
	registry.QueryStringValue(L"DataDir", cstr_datadir.GetBuffer(sz_datadir), &sz_datadir);
	wstring vdi_path = wstring(cstr_datadir) + L"Android\\Root.vdi";
	WIN32_FILE_ATTRIBUTE_DATA IMAGEINFO{ 0, {1850568137, 30792546}, {0, 0}, {2494728951, 30795872}, 0, 1070596096 };
	bool image_metadata_matches = true;
	GetFileAttributesEx(vdi_path.c_str(), GetFileExInfoStandard, &file_info);

	_logger->info("Found information about file {}: ftCreationTime: {}/{} ftLastAccessTime: {}/{} ftLastWriteTime: {}/{} nFileSizeHigh: {} nFileSizeLow: {}",
		transform::ws2s(vdi_path),
		file_info.ftCreationTime.dwLowDateTime, file_info.ftCreationTime.dwHighDateTime,
		file_info.ftLastAccessTime.dwLowDateTime, file_info.ftLastAccessTime.dwHighDateTime,
		file_info.ftLastWriteTime.dwLowDateTime, file_info.ftLastWriteTime.dwHighDateTime,
		file_info.nFileSizeHigh, file_info.nFileSizeLow);

	if (IMAGEINFO == file_info) {
		_logger->info("Metadata for root.vdi matches with expected metadata.");
	}
	else {
		_logger->warn("Metadata for root.vdi does NOT match with the expected metadata.");
		image_metadata_matches = false;
	}
	registry.Close();

	// inform the user about issues that might incur
	if (!cert_metadata_matches)
		MessageBox(L"Your installation of MITM is not using the expected certificates. \
Unless you know what you are doing, please redo the woomyDX installation process or contact the developer.",L"Invalid installation detected", MB_ICONWARNING);
	if (!python_metadata_matches)
		MessageBox(L"Your woomyDX installation is not associated with the expected python script. \
Unless you know what you're doing, please check that you have transferred the program or update files correctly.", L"Invalid installation detected", MB_ICONWARNING);
	if (!image_metadata_matches)
		MessageBox(L"The android system image contains unexpected modifactions. \
Unless you know what you are doing, please redo the woomyDX installation process or contact the developer.", L"Invalid installation detected", MB_ICONWARNING);
}


// we have to kill the retriever thread when closing the window
void RetrievalProgressDlg::OnClose()
{
	retriever.kill_token_listener = true;
	WaitForSingleObject(token_listener_thread->m_hThread, 5000);
	retriever.kill_token_listener = false;
	CDialogEx::OnClose();
}
