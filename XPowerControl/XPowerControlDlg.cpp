
// XPowerControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XPowerControl.h"
#include "XPowerControlDlg.h"
#include "RetrieveTokenDlg.h"
#include "CSettingsDialog.h"
#include "afxdialogex.h"
#include "monitor.h"
#include "rotation_monitor.h"
#include "http_requests.h"
#include "settings.h"
#include "NewTokenDlg.h"
#include "analytics.h"
#include "FirstSetupDlg.h"
#include "Version.h"
#include "http_requests.h"
#include "wstring_transform.h"
#include "logging.h"
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <iterator>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/* TODO:
- Scaling issues when scaling is set to above 100%
- MITM installation is exited a bit early, sometimes the uninstaller doesn't get installed properly
- Program window opens when closing "Action needed" popup
- When there's an update pending, the pressing Next on the First Setup Dlg has a little delay
- proxy installation unreliable
*/

// CXPowerControlDlg dialog

using namespace std;

CXPowerControlDlg::CXPowerControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_XPOWERCONTROL_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	_logger = logging::get_logger(DEFAULT_LOG);
}

void CXPowerControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOADING, m_loading_info);
	DDX_Control(pDX, IDC_BATTLE_START_TEXT, m_battle_start_text);
}

BEGIN_MESSAGE_MAP(CXPowerControlDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
//	ON_WM_ERASEBKGND()
ON_BN_CLICKED(IDC_BUTTON1, &CXPowerControlDlg::OnBnClickedButton1)
ON_STN_CLICKED(IDC_LOSEVALUE, &CXPowerControlDlg::OnStnClickedLosevalue)
//ON_ACN_START(IDC_ANIMATE1, &CXPowerControlDlg::OnAcnStartAnimate1)
ON_WM_CLOSE()
ON_STN_CLICKED(IDC_BATTLE_START_TEXT, &CXPowerControlDlg::OnStnClickedBattleStartText)
ON_STN_CLICKED(IDC_LOSEDETAILS, &CXPowerControlDlg::OnStnClickedLosedetails)
ON_STN_CLICKED(IDC_STATICBTN_SETTINGS, &CXPowerControlDlg::OnStnClickedSettings)
ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

bool CXPowerControlDlg::start_monitoring() {
	_logger->info("Sending signal to start monitoring.");
	// run game monitor
	string SESSID = read_from_settings<string>("iksm-session");

	if (SESSID == "") {
		NewTokenDlg dlg(INFO_NOIKSM);
		dlg.DoModal();
	}
	else {

		// test run a call to chech whether the sessid is valid
		string schedule_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/schedules", SESSID);
		// if the string contains "AUTHENTICATION_ERROR", we quit
		if (schedule_json_string.find("AUTHENTICATION_ERROR") != std::string::npos) {
			NewTokenDlg dlg(INFO_EXPIRED);
			dlg.DoModal();
			start_monitoring();
			return TRUE;
		}

		thread_rotation_monitor = AfxBeginThread(monitor_rotation, this);
		thread_monitor_main = AfxBeginThread(monitor_main_alt, this);
		_logger->info("Started montioring");
		return TRUE;  // return TRUE  unless you set the focus to a control
	}
}



void  CXPowerControlDlg::stop_monitoring() {
	_logger->info("Sending signal to stop monitoring.");
	kill_monitor_main = true;
	kill_rotation_monitor = true;
	DWORD main_response = WaitForSingleObject(thread_monitor_main->m_hThread, 5000);
	DWORD rotation_response = WaitForSingleObject(thread_rotation_monitor->m_hThread, 5000);
	_logger->info("Stopped monitoring");
	kill_monitor_main = false;
	kill_rotation_monitor = false;
}

// CXPowerControlDlg message handlers

UINT thread_update_info(LPVOID pParam) {
	CXPowerControlDlg* dlg = (CXPowerControlDlg*)pParam;

	dlg->GetDlgItem(IDC_STATIC_UPDATE)->ShowWindow(SW_SHOW);
	for (int i = 0; i < 2; ++i) {
		dlg->GetDlgItem(IDC_STATIC_UPDATE)->SetWindowText(L"Update Available!");
		dlg->Invalidate();
		dlg->UpdateWindow();
		Sleep(1500);
		dlg->GetDlgItem(IDC_STATIC_UPDATE)->SetWindowText((L"Your version: " + dlg->this_ver.to_wstring(3)).c_str());
		dlg->Invalidate();
		dlg->UpdateWindow();
		Sleep(1500);
		dlg->GetDlgItem(IDC_STATIC_UPDATE)->SetWindowText((L"Latest version: " + dlg->latest_ver.to_wstring(3)).c_str());
		dlg->Invalidate();
		dlg->UpdateWindow();
		Sleep(1500);
	}
	dlg->GetDlgItem(IDC_STATIC_UPDATE)->ShowWindow(SW_HIDE);

	return 0;
}



BOOL CXPowerControlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	RECT rect_client;
	GetClientRect(&rect_client);
	staticbtn_settings.Create(_T(""),
		WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE | SS_NOTIFY | SS_REALSIZEIMAGE,
		CRect(rect_client.right-25, rect_client.bottom-25, rect_client.right, rect_client.bottom), this, IDC_STATICBTN_SETTINGS);

	// TODO: Add extra initialization here
	CFont *powerFont = new CFont();
	powerFont->CreateFont(
		72,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		500,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		ANTIALIASED_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		_T("Calibri Light"));                 // lpszFacename

	CFont *powerFontSmall = new CFont();
	powerFontSmall->CreateFont(
		34,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		500,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		ANTIALIASED_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		_T("Calibri Light"));                 // lpszFacename

	CFont *font_bold = new CFont();
	font_bold->CreateFont(
		22,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		500,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		ANTIALIASED_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		_T("Calibri Light"));                 // lpszFacename

	CFont *font_default = new CFont();
	font_default->CreateFont(
		20,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		400,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		ANTIALIASED_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		_T("Calibri Light"));                 // lpszFacename

	GetDlgItem(IDC_WINVALUE)->SetFont(powerFont);
	GetDlgItem(IDC_LOSEVALUE)->SetFont(powerFont);
	GetDlgItem(IDC_POWERVALUE)->SetFont(powerFontSmall);
	GetDlgItem(IDC_WINTEXT)->SetFont(font_bold);
	GetDlgItem(IDC_LOSETEXT)->SetFont(font_bold);
	GetDlgItem(IDC_LOADING)->SetFont(font_default);
	GetDlgItem(IDC_BATTLE_START_TEXT)->SetFont(font_default);
	GetDlgItem(IDC_STATIC_UPDATE)->SetFont(font_default);

#ifdef DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "This is the debugging window. If you're not the developer of this software and you can see this, please contact the developer and tell her that she's a dumb dumb.";
#endif

	// check if version is up to date
	TCHAR exec_filename[MAX_PATH];
	GetModuleFileName(NULL, exec_filename, MAX_PATH);
	this_ver = Version::from_file(exec_filename);
	latest_ver = Version::from_string(http_requests::get_global_info("latest_version"));

	if (this_ver < latest_ver) {
		AfxBeginThread(thread_update_info, this);
	}

#ifdef DEBUG
	SetWindowText((L"woomyDX v" + this_ver.to_wstring(3) + L"dev").c_str());
#else
	SetWindowText((L"woomyDX v" + this_ver.to_wstring(3) + L" [sub-only]").c_str());
#endif
	

	// check validity of the settings

	// See if settings.txt exists
	if (!filesystem::exists("settings.txt")) {
		AfxMessageBox(_T("Before you can run this program, you have to set up external software to retrieve an access token. Press OK to start the setup wizard."));
		ofstream out_file("settings.txt");
		out_file << "{}";
		out_file.close();
	}

	// Try to parse settings.txt as JSON
	int attempt_num = 1; // if the settings file seems to be corrupted, we give it three attempts to make sure we're not falsely deleting data
	while (attempt_num <= 3) {
		try {
			attempt_num++;
			nlohmann::json j;
			ifstream in_file("settings.txt");
			if (!in_file.good()) { // if we couldn't open the file, throw error
				_logger->error("FATAL: Access to settings.txt file was denied.");
				AfxMessageBox(L"Fatal error: Failed to open the settings file.");
				AfxThrowUserException();
			}
			in_file >> j;
			in_file.close();
			break;
		}
		catch (exception e) {
			if (attempt_num <= 3)
				continue;
			ifstream in_file("settings.txt");
			string settings_str{ std::istreambuf_iterator<char>{in_file},{} }; // uniform-initialization to avoid being parsed as function declaration
			_logger->error("Settings file was reported as corrupted.\ne.what() returned: {}\nsettings file content: {}", e.what(), settings_str);
			in_file.close();
			AfxMessageBox(_T("Your settings file is corrupted. A new file will be created, but there might be an error in your woomyDX installation."));
			ofstream out_file("settings.txt");
			out_file << "{}";
			out_file.close();
		}
	}

	// we try to read the settings version from the file and confirm that it is up to date
	bool is_current_version = false;
	int settings_version = 0;
	nlohmann::json j;
	if (settings_exist("settings_version")) {
		settings_version = read_from_settings<int>("settings_version");
		if (settings_version == CURRENT_SETTING_VERSION)
			is_current_version = true;
	}

	if (!is_current_version) { // if the version is not up to date, we transfer the data

		j["settings_version"] = CURRENT_SETTING_VERSION;
		j["iksm-session"] = read_from_settings<string>("iksm_session","");
		j["mitm_exec"] = read_from_settings<string>("mitm_exec", "");
		j["emu_exec"] = read_from_settings<string>("emu_exec", "");
		j["emu_cmd"] = read_from_settings<string>("emu_cmd", "");
		j["matchdata_directory"] = read_from_settings<string>("matchdata_directory", "./live_output/");
		j["start_cmd"] = read_from_settings<string>("start_cmd", "");
		j["end_cmd"] = read_from_settings<string>("end_cmd", "");
		j["max_precise"] = read_from_settings<int>("max_precise", 40);
		j["do_savematchdata"] = read_from_settings<bool>("do_savematchdata", true);
		j["do_startcmd"] = read_from_settings<bool>("do_startcmd", false);
		j["do_endcmd"] = read_from_settings<bool>("do_endcmd", false);
		j["do_hidepower"] = read_from_settings<bool>("do_hidepower", true);
		j["do_ontop"] = read_from_settings<bool>("do_ontop", true);
		j["performed_setup"] = read_from_settings<bool>("performed_setup", false);
		j["latest_nso_version"] = read_from_settings<string>("latest_nso_version", "0.0.0.0");

		ofstream out_file("settings.txt");
		out_file << j;
		out_file.close();
	}

	// make sure that directory in matchdata_directory actually exists
	string matchdata_dir = read_from_settings<string>("matchdata_directory");
	auto dir_status = filesystem::status(matchdata_dir);
	if (!filesystem::exists(matchdata_dir)) {
		bool create_success = filesystem::create_directory(matchdata_dir);
		if (!create_success) {
			wstring message = L"The program tried to create a folder for your intended stream file directory" + transform::s2ws(matchdata_dir)\
				+ L"\nbut failed to do so.\nThis program will run regardless, but stream files will not be created.";\
			MessageBox(message.c_str(), L"Stream file error", MB_OK | MB_ICONWARNING);
			_logger->warn("Sent warning to user: {}", transform::ws2s(message));
		}
	}
	else if (!filesystem::is_directory(matchdata_dir)) {
		wstring message = L"The intended directory for you stream output files \"" + transform::s2ws(matchdata_dir) + L"\" was found, but it seems to be a file, not a directory.\n\
This program will run regardless, but stream files will not be created.";
		MessageBox(message.c_str(), L"Stream file error", MB_OK | MB_ICONWARNING);
		_logger->warn("Sent warning to user: {}", transform::ws2s(message));
	}

	if (!read_from_settings<bool>("performed_setup")) {
		_logger->info("Starting initial setup.");
		FirstSetupDlg dlg;
		dlg.DoModal();
	}


	if (read_from_settings<bool>("do_ontop"))
		SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	start_monitoring();

	return true;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXPowerControlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		// we obtain the window size
		CRect size;
		GetClientRect(&size);
		int winHeight = size.Height();
		int winSplit = size.Width() / 2;
		int winWidth = size.Width();

		CPaintDC dc(this);

		// create brushes and pens
		// dark mode: red #8F533F, green #435C2E, blue #35448F -- NOT IMPLEMENTED
		// light mode: red #FFD5C7, green #E3FFCC, blue #D9DFFF
		CPen penGreen, penRed;
		CBrush brushGreen, brushRed;
		penGreen.CreatePen(PS_SOLID, 1, RGB(227, 255,204));
		penRed.CreatePen(PS_SOLID, 1, RGB(255, 213, 199));
		brushGreen.CreateSolidBrush(RGB(227, 255, 204));
		brushRed.CreateSolidBrush(RGB(255, 213, 199));

		// select brush and pen
		dc.SelectObject(&penGreen);
		dc.SelectObject(&brushGreen);
		// paint background rectangle
		dc.Rectangle(0, 0, winSplit, winHeight-1);

		// select brush and pen
		dc.SelectObject(&penRed);
		dc.SelectObject(&brushRed);
		// paint background rectangle
		dc.Rectangle(winSplit+1, 0, winWidth, winHeight);

		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXPowerControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

HBRUSH CXPowerControlDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{

	switch (nCtlColor)
	{
	case CTLCOLOR_STATIC:
		// add settings wheel to settings button
		if (pWnd->GetDlgCtrlID() == IDC_STATICBTN_SETTINGS) {
			Gdiplus::Bitmap* settings_bitmap;
			HICON settings_icon;
			settings_bitmap = Gdiplus::Bitmap::FromFile(L"settings.png");
			settings_bitmap->GetHICON(&settings_icon);
			DrawIconEx(*pDC, 0, 0, settings_icon,25,25,0,NULL, DI_NORMAL | DI_COMPAT);
			DestroyIcon(settings_icon);
		}
		if (pWnd->GetDlgCtrlID() == IDC_STATIC_UPDATE) {
			pDC->SetTextColor(RGB(255, 0, 0));
		}
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	default:
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}


//BOOL CXPowerControlDlg::OnEraseBkgnd(CDC* pDC)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	return CDialogEx::OnEraseBkgnd(pDC);
//}


void CXPowerControlDlg::OnBnClickedButton1()
{
	RetrieveTokenDlg dialog;
	dialog.DoModal();
	// TODO: Add your control notification handler code here
}


void CXPowerControlDlg::OnStnClickedLosevalue()
{
	// TODO: Add your control notification handler code here
}



void CXPowerControlDlg::OnClose()
{
	stop_monitoring();
	Gdiplus::GdiplusShutdown(gdiplusToken);
	CDialogEx::OnClose();
}


void CXPowerControlDlg::OnStnClickedBattleStartText()
{
	// TODO: Add your control notification handler code here
}


void CXPowerControlDlg::OnStnClickedLosedetails()
{
	// TODO: Add your control notification handler code here
}


void CXPowerControlDlg::OnStnClickedSettings() {
	CSettingsDialog settings_dlg;
	settings_dlg.DoModal();
	stop_monitoring();
	start_monitoring();
}


void CXPowerControlDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialogEx::OnWindowPosChanged(lpwndpos);
	if (IsWindowVisible() == true && read_from_settings<bool>("do_ontop"))
		SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
}

void CXPowerControlDlg::refresh_static_text(int item_t, std::wstring text_t) // WARNING: This doesn't work
{
	GetDlgItem(item_t)->SetWindowTextW(text_t.c_str());
	CRect client_rect;
	GetDlgItem(item_t)->GetClientRect(&client_rect);
	InvalidateRect(client_rect, true);
	UpdateWindow();
}
