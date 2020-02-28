// FirstSetupDlg.cpp : implementation file
//

#include "FirstSetupDlg.h"
#include "FirstSetupDlg2.h"
#include "afxdialogex.h"
#include "Version.h" 
#include "wstring_transform.h"
#include <assert.h>
#include <string>
#include <optional>
#include <boost/algorithm/string/erase.hpp>
#include "http_requests.h"
#include "nlohmann/json.hpp"
#include "ActionRequiredDlg.h"

#define REGISTRY_PATH  L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define GIGABYTE (unsigned __int64) 1000000000

using namespace std;
// FirstSetupDlg dialog

IMPLEMENT_DYNAMIC(FirstSetupDlg, CDialogEx)

FirstSetupDlg::FirstSetupDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FIRSTSETUP, pParent)
{

}

FirstSetupDlg::~FirstSetupDlg()
{
}

void FirstSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(FirstSetupDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &FirstSetupDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &FirstSetupDlg::OnBnClickedCancel)
	ON_WM_CTLCOLOR()
	ON_STN_CLICKED(IDC_STATIC_INSUFFICIENT_STORAGE, &FirstSetupDlg::OnStnClickedStaticInsufficientStorage)
	ON_BN_CLICKED(IDC_BUTTON1, &FirstSetupDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// FirstSetupDlg message handlers

BOOL IsProcessElevated()
{
	BOOL fIsElevated = FALSE;
	HANDLE hToken = NULL;
	TOKEN_ELEVATION elevation;
	DWORD dwSize;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		printf("\n Failed to get Process Token :%d.", GetLastError());
		goto Cleanup;  // if Failed, we treat as False
	}


	if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
	{
		printf("\nFailed to get Token Information :%d.", GetLastError());
		goto Cleanup;// if Failed, we treat as False
	}

	fIsElevated = elevation.TokenIsElevated;

Cleanup:
	if (hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}
	return fIsElevated;
}


// like Sleep(), but aborts if a flag has been set
bool sleep_or_kill(int milliseconds, bool* check) {
	int step = min(milliseconds, 10);
	for (int i = 0; i <= milliseconds; i += step) {
		Sleep(step);
		if (*check)
			return true;
	}

	return *check;
}

UINT thread_update_info_setup(LPVOID pParam) {
	FirstSetupDlg* dlg = (FirstSetupDlg*)pParam;

	dlg->GetDlgItem(IDC_STATIC_UPDATE2)->ShowWindow(SW_SHOW);
	while (!dlg->kill_update_info) {
		dlg->GetDlgItem(IDC_STATIC_UPDATE2)->SetWindowText(L"Update Available!");
		dlg->Invalidate();
		dlg->UpdateWindow();
		if (sleep_or_kill(1500, &dlg->kill_update_info))
			break;
		dlg->GetDlgItem(IDC_STATIC_UPDATE2)->SetWindowText((L"Your version: " + dlg->this_ver.to_wstring(3)).c_str());
		dlg->Invalidate();
		dlg->UpdateWindow();
		if (sleep_or_kill(1500, &dlg->kill_update_info))
			break;
		dlg->GetDlgItem(IDC_STATIC_UPDATE2)->SetWindowText((L"Latest version: " + dlg->latest_ver.to_wstring(3)).c_str());
		dlg->Invalidate();
		dlg->UpdateWindow();
		if (sleep_or_kill(1500, &dlg->kill_update_info))
			break;
	}
	dlg->GetDlgItem(IDC_STATIC_UPDATE2)->ShowWindow(SW_HIDE);

	return 0;
}


BOOL FirstSetupDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	// check if version is up to date
	TCHAR exec_filename[MAX_PATH];
	GetModuleFileName(NULL, exec_filename, MAX_PATH);
	this_ver = Version::from_file(exec_filename);

	string version_page = http_requests::load_page("https://raw.githubusercontent.com/snowpoke/XPowerControl-public/master/version.json", "");
	boost::erase_all(version_page, "\n");
	auto j_version = nlohmann::json::parse(version_page);
	latest_ver = Version::from_string(j_version["latest_version"]);

	if (this_ver < latest_ver) {
		update_info_monitor = AfxBeginThread(thread_update_info_setup, this);
	}

	// get free space on current drive
	ULARGE_INTEGER free_space;
	GetDiskFreeSpaceEx(NULL, &free_space, NULL, NULL);

	// check if "program files" path and current program path are on different drives
	wchar_t program_files_path[MAX_PATH];
	wchar_t this_program_path[MAX_PATH];

	SHGetSpecialFolderPath(0, program_files_path, CSIDL_PROGRAM_FILES, FALSE);
	GetModuleFileName(NULL, this_program_path, MAX_PATH);

	wstring w_program_files_path = wstring(program_files_path);
	wstring w_this_program_path = wstring(this_program_path);

	auto pos1 = w_program_files_path.find(L'\\');
	auto pos2 = w_this_program_path.find(L'\\');

	wstring program_files_drive(w_program_files_path.begin(), w_program_files_path.begin() + pos1 + 1);
	wstring this_program_drive(w_this_program_path.begin(), w_this_program_path.begin() + pos2 + 1);

	if (program_files_drive == this_program_drive) {
		// everything is installed on the same drive - 4.5GB of space are required
		if (free_space.QuadPart < 4.5 * GIGABYTE) { // on 64bit systems QuadPart contains the value we need
			GetDlgItem(IDC_STATIC_INSUFFICIENT_STORAGE)->SetWindowText((L"Error! You don't have enough disk space to set up this program.\n\
To install helper software, you will need 4.5GB on drive " + this_program_drive + L".\n\
(2GB temporary installer files + 2.5GB permanent installations)").c_str());
			GetDlgItem(IDC_STATIC_INSUFFICIENT_STORAGE)->ShowWindow(SW_SHOWNORMAL);
			GetDlgItem(IDOK)->EnableWindow(false);
		}
	}
	else {
		// get free space on drive for Program Files
		ULARGE_INTEGER free_space_prog;
		GetDiskFreeSpaceEx(NULL, &free_space_prog, NULL, NULL);

		if (free_space.QuadPart < 2 * GIGABYTE || free_space_prog.QuadPart < 2.5 * GIGABYTE) {
			GetDlgItem(IDC_STATIC_INSUFFICIENT_STORAGE)->SetWindowText((L"Error! You don't have enough disk space to set up this program.\n\
To install helper software, you will need 2GB on drive " + this_program_drive + L" for temporary files\n\
and 2.5GB of space on drive " + program_files_drive + L" for permanent installations").c_str());
			GetDlgItem(IDC_STATIC_INSUFFICIENT_STORAGE)->ShowWindow(SW_SHOWNORMAL);
			GetDlgItem(IDOK)->EnableWindow(false);
		}
	}


	// set font for the dialog title
	CFont* font_dlg = GetDlgItem(IDC_STATIC_TITLE)->GetFont();
	LOGFONT logfont_bold;
	font_dlg->GetLogFont(&logfont_bold);
	logfont_bold.lfWeight = 700;
	logfont_bold.lfHeight = 14;
	CFont* font_bold = new CFont();
	font_bold->CreateFontIndirectW(&logfont_bold);
	GetDlgItem(IDC_STATIC_TITLE)->SetFont(font_bold);

	// if we don't have admin rights, restart program with admin rights (in debug, we expect admin rights)
#ifdef NDEBUG
	if (!IsProcessElevated()) {
		wchar_t exec_filename[MAX_PATH];
		DWORD filename_size = MAX_PATH;
		GetModuleFileName(NULL, exec_filename, filename_size);
		ShellExecute(NULL, L"runas", exec_filename, NULL, NULL, SW_SHOWNORMAL);
		exit(0);
	}
#endif

	// check if nox and mitmproxy are already installed
	CRegKey registry;
	optional<CString> bs_version_str;
	optional<CString> mitm_version_str;
	// open registry key
	ULONG sz_version = 20;
	// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\BlueStacks has info we need in DataDir and InstallDir

	auto bs_key_ret = registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\BlueStacks", KEY_READ);
	if (bs_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		bs_version_str = CString();
		registry.QueryStringValue(L"ClientVersion", (*bs_version_str).GetBuffer(sz_version), &sz_version);
	}
	
	auto mitm_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\mitmproxy 5.0.1", KEY_READ);
	if (mitm_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		mitm_version_str = CString();
		registry.QueryStringValue(L"DisplayVersion", (*mitm_version_str).GetBuffer(sz_version), &sz_version);
	}

	if (bs_version_str || mitm_version_str) {
		GetDlgItem(IDC_STATIC_NOTE)->ShowWindow(SW_SHOW);
	}
	
	// if BlueStacks is already installed we check the BlueStacks version to see if we need to update
	if (bs_version_str) {

		Version bs_version = Version::from_string(transform::ws2s(wstring(*bs_version_str)));
		Version bs_min_version = Version::from_int(4, 170, 10, 1001);
		
		needs_bs = (bs_min_version > bs_version);
	}
	
	// if mitmproxy is installed, we don't need to update in any case
	if (mitm_version_str)
		needs_mitm = false;

	return true;
}

void FirstSetupDlg::OnBnClickedOk()
{
	if (this_ver < latest_ver) { // end thread that rotates update info
		kill_update_info = true;
		WaitForSingleObject(update_info_monitor->m_hThread, 2000);
		kill_update_info = false;
	}

	ShowWindow(SW_HIDE);
	FirstSetupDlg2 dlg(needs_bs, needs_mitm);
	dlg.DoModal();
	CDialogEx::OnOK();
}


void FirstSetupDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
	PostQuitMessage(0);
}


HBRUSH FirstSetupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	switch (nCtlColor)
	{
	case CTLCOLOR_STATIC:
		if (pWnd->GetDlgCtrlID() == IDC_STATIC_INSUFFICIENT_STORAGE)
			pDC->SetTextColor(RGB(255, 0, 0));
		return hbr;
	default:
		return hbr;
	}
}


void FirstSetupDlg::OnStnClickedStaticInsufficientStorage()
{
	// TODO: Add your control notification handler code here
}


void FirstSetupDlg::OnBnClickedButton1()
{
	ActionRequiredDlg dlg("mitm_install.bmp",L"This is just a test.");
}
