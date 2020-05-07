// UpdateNSODlg.cpp : implementation file
//

#include "afxdialogex.h"
#include "UpdateNSODlg.h"
#include "CustomDialogBase.h"
#include "settings.h"
#include "http_requests.h"
#include "wstring_transform.h"
#include <boost/format.hpp>
#include "FirstSetup.h"
#include "TokenRetriever.h"
#include "logging.h"

using namespace std;
// UpdateNSODlg dialog

IMPLEMENT_DYNAMIC(UpdateNSODlg, CDialogEx)

UpdateNSODlg::UpdateNSODlg(CWnd* pParent /*=nullptr*/)
	: CustomDialogBase(IDD_UPDATE_NSO, pParent)
{
	register_message_mapping(MSG_UPDATE_DL_PROGRESS, (std::function<void(void*)>) on_update_dl_progress_wrapper);
}

UpdateNSODlg::~UpdateNSODlg()
{
}


void UpdateNSODlg::DoDataExchange(CDataExchange* pDX)
{
	CustomDialogBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_NSODL, progressbar_nso);
}


BEGIN_MESSAGE_MAP(UpdateNSODlg, CustomDialogBase)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// UpdateNSODlg message handlers

bool UpdateNSODlg::is_update_required()
{
	Version curr_version, latest_version;
	return is_update_required(curr_version, latest_version);
}


bool UpdateNSODlg::is_update_required(Version& curr_version_t, Version& latest_version_t)
{
	// read current NSO version from settings file
	string str_installed_nso_version = read_from_settings<string>("latest_nso_version");
	curr_version_t = Version::from_string(str_installed_nso_version);

	// read latest NSO version from online JSON file
	string str_latest_nso_version = http_requests::get_global_info("latest_nso_version");
	latest_version_t = Version::from_string(str_latest_nso_version);

	// add results of this assessment to the log
	auto _logger = logging::get_logger(DEFAULT_LOG);
	_logger->info("Running is_update_required. Current NSO version: {} - Latest NSO version: {}", str_installed_nso_version, str_latest_nso_version);

	return (latest_version_t > curr_version_t);
}

void UpdateNSODlg::on_update_dl_progress_wrapper(void* pParam) // static wrapper function
{
	UpdateNSODlg* dlg = (UpdateNSODlg*)pParam;
	dlg->on_update_dl_progress();
}

void UpdateNSODlg::on_update_dl_progress()
{
	long dl_progress = dl_callback.progress;
	long dl_max = dl_callback.max_dl;

	// Set progress bar position
	int pos = dl_max > 0 ? floor(static_cast<long double>(dl_progress) * 100.0 / static_cast<long double>(dl_max)) : 0;
	progressbar_nso.SetPos(min(pos,99));

	// Set description text
	boost::format mb_format("%.1fMB");
	string dl_progress_mb = (mb_format % (static_cast<long double>(dl_progress) / 1000000.0)).str();
	string dl_max_mb = (mb_format % (static_cast<long double>(dl_max) / 1000000.0)).str();
	wstring status_message = L"Loaded " + transform::s2ws(dl_progress_mb) + L" out of " + transform::s2ws(dl_max_mb) + L".";
	GetDlgItem(IDC_STATIC_NSOINSTALL)->SetWindowTextW(status_message.c_str());

	// after the download has finished, install the apk
	if (dl_callback.finished) {
		CRegKey registry;
		ULONG sz_installdir = MAX_PATH;
		CString cstr_installdir = CString();

		// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\BlueStacks has info we need in InstallDir
		registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
		registry.QueryStringValue(L"InstallDir", cstr_installdir.GetBuffer(sz_installdir), &sz_installdir);
		wstring installdir = wstring(cstr_installdir);

		// keep running proxy setup until it returns success code
		DWORD exit_code;
		HANDLE process_handle;
		HANDLE exec_handle;

		GetDlgItem(IDC_STATIC_NSOINSTALL)->SetWindowTextW(L"Installing Nintendo Switch Online app...");

		// run code to install NSO app
		exec_handle = TokenRetriever::run_command(installdir + L"HD-ApkHandler.exe nso.apk");
		process_handle = FirstSetup::job_to_process_handle(exec_handle);
		WaitForSingleObject(process_handle, INFINITE);
		GetExitCodeProcess(process_handle, &exit_code);

		auto _logger = logging::get_logger(DEFAULT_LOG);
		_logger->info("Installed NSO app with exit code {}.", to_string(exit_code));

		progressbar_nso.SetPos(100);
		GetDlgItem(IDC_STATIC_NSOINSTALL)->SetWindowTextW(L"Installation succeeded!");

		// update settings to say that the latest version of NSO is installed
		ifstream in_file("settings.txt");
		nlohmann::json j;
		in_file >> j;
		in_file.close();

		j["latest_nso_version"] = transform::ws2s(latest_version.to_wstring(4));

		ofstream out_file("settings.txt");
		out_file << j;

		out_file.close();

		Sleep(100); // we give the viewer a little bit of time to see the message, to avoid confusion
		SendMessage(WM_CLOSE);
	}
	
}

BOOL UpdateNSODlg::OnInitDialog()
{
	CustomDialogBase::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	bool update_required = is_update_required(curr_version, latest_version);

	// update version into on dialog
	GetDlgItem(IDC_STATIC_CURRVER)->SetWindowTextW(curr_version.to_wstring(4).c_str());
	GetDlgItem(IDC_STATIC_LATESTVER)->SetWindowTextW(latest_version.to_wstring(4).c_str());

	if (update_required) {

		// download apk in separate thread, callback sends messages to window
		wstring nso_dl_link = transform::s2ws(http_requests::get_global_info("nso_dl_link"));
		dl_callback = http_requests::Callback(this);

		dl_url = nso_dl_link.data();
		dl_param.dl_URL = dl_url;
		dl_param.savepath_t = L"nso.apk";
		dl_param.callback_t = &dl_callback;

		AfxBeginThread(http_requests::thread_download_file, &dl_param);

	}

	return true;
}

void UpdateNSODlg::OnClose()
{
	// end message listener
	// TODO: This should really be in CustomDialogBase, but I haven't figured out how to make it support OnClose
	send_message(MSG_STOP_LISTENER);
	WaitForSingleObject(winthread_message_listener->m_hThread, 1000);

	// abort download if necessary
	dl_callback.cancel();

	CustomDialogBase::OnClose();
}
