// NewTokenDlg.cpp : implementation file
//

#include "NewTokenDlg.h"
#include "SetupRetrievalDlg.h"
#include "RetrievalProgressDlg.h"
#include "afxdialogex.h"
#include "settings.h"
#include <string>
#include "http_requests.h"
#include <gdiplus.h>

using namespace std;
// NewTokenDlg dialog

IMPLEMENT_DYNAMIC(NewTokenDlg, CDialogEx)

NewTokenDlg::NewTokenDlg(UINT token_info_t /*=INFO_EXPIRED*/, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NEW_TOKEN, pParent)
{
	token_info = token_info_t;
}

NewTokenDlg::NewTokenDlg(CSettingsDialog* dlg_t, UINT token_info_t /*=INFO_EXPIRED*/, CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_NEW_TOKEN, pParent)
{
	settings_dlg = dlg_t;
	token_info = token_info_t;
}

NewTokenDlg::~NewTokenDlg()
{
}

void NewTokenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_START, button_iksm_start);
	DDX_Control(pDX, IDC_EDIT_MANUAL, edit_manual);
	DDX_Control(pDX, IDOK, button_ok);
}


BEGIN_MESSAGE_MAP(NewTokenDlg, CDialogEx)
//	ON_WM_SHOWWINDOW()
ON_BN_CLICKED(IDC_BUTTON_START, &NewTokenDlg::OnBnClickedButtonStart)
ON_WM_CTLCOLOR()
ON_EN_CHANGE(IDC_EDIT_MANUAL, &NewTokenDlg::OnEnChangeEditManual)
ON_BN_CLICKED(IDOK, &NewTokenDlg::OnBnClickedOk)
ON_STN_CLICKED(IDC_STATIC_SETUP_LINK, &NewTokenDlg::OnStnClickedStaticSetupLink)
ON_STN_CLICKED(IDC_STATICBTN_SETTINGS, &NewTokenDlg::OnStnClickedSettings)
ON_WM_CLOSE()
ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// NewTokenDlg message handlers

BOOL NewTokenDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	staticbtn_settings.Create(_T(""),
		WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE | SS_NOTIFY | SS_REALSIZEIMAGE,
		CRect(410, 147, 435, 172), this, IDC_STATICBTN_SETTINGS);

	CFont* font_dlg = GetDlgItem(IDC_STATIC_RUNOUT_INFO)->GetFont();
	LOGFONT logfont_bold, logfont_underline;
	font_dlg->GetLogFont(&logfont_bold);
	logfont_bold.lfWeight = 700;
	CFont* font_bold = new CFont();
	font_bold->CreateFontIndirectW(&logfont_bold);

	// set info panel's text depending on token_info
	switch (token_info) {
	case INFO_NOIKSM:
		GetDlgItem(IDC_STATIC_RUNOUT_INFO)->SetWindowTextW(L"Please set up an IKSM token.");
		break;
	case INFO_EXPIRED:
		GetDlgItem(IDC_STATIC_RUNOUT_INFO)->SetWindowTextW(L"Your IKSM token has run out!");
		break;
	default:
		GetDlgItem(IDC_STATIC_RUNOUT_INFO)->SetWindowTextW(L"");
		break;
	}

	font_dlg->GetLogFont(&logfont_underline);
	logfont_underline.lfUnderline = true;
	CFont* font_underline = new CFont();
	font_underline->CreateFontIndirectW(&logfont_underline);

	GetDlgItem(IDC_STATIC_RUNOUT_INFO)->SetFont(font_bold);
	GetDlgItem(IDC_STATIC_SETUP_LINK)->SetFont(font_underline);

	// create cursors for retrieval setup link
	cursor_hand = LoadCursor(NULL, IDC_HAND);
	cursor_arrow = LoadCursor(NULL, IDC_ARROW);

	// if IKSM Token retrieval is set up, enable elements
	if (read_from_settings<string>("mitm_exec") != "" && read_from_settings<string>("emu_cmd") != "") {
		button_iksm_start.EnableWindow(true);
		GetDlgItem(IDC_STATIC_NOTSETUP)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_SETUP_LINK)->ShowWindow(SW_HIDE);
	}

	return 1;
}

struct Retrieval_success_listener_params {
	RetrievalProgressDlg* progress_dlg;
	NewTokenDlg* token_dlg;
};

UINT retrieval_success_listener(LPVOID pParam) {
	Retrieval_success_listener_params* params = (Retrieval_success_listener_params*) pParam;
	RetrievalProgressDlg* progress_dlg = params->progress_dlg;
	NewTokenDlg* token_dlg = params->token_dlg;

	while (progress_dlg->has_succeeded == false) { // wait until success signal is sent
		Sleep(1);
	}

	if (progress_dlg->has_succeeded) {
		if (IsWindow(progress_dlg->GetSafeHwnd())) // don't do this if user clicked 'cancel'
			progress_dlg->PostMessage(WM_CLOSE);
		token_dlg->GetDlgItem(IDC_STATIC_SUCCESS)->ShowWindow(SW_SHOW);
		token_dlg->GetDlgItem(IDC_STATIC_RUNOUT_INFO)->ShowWindow(SW_HIDE);
		token_dlg->GetDlgItem(IDOK)->EnableWindow(true);

		// if this window was called by CSettingsDialog, send MSG_REFRESH_SETTINGS
		if (token_dlg->settings_dlg)
			token_dlg->settings_dlg.value()->send_message(MSG_REFRESH_SETTINGS);
	}

	return 0;
}

void NewTokenDlg::OnBnClickedButtonStart() {
	RetrievalProgressDlg progress_dlg;
	progress_dlg.no_closing_after_success = true; // we're going to close the dialog from here
	Retrieval_success_listener_params params = { &progress_dlg, this };
	AfxBeginThread(retrieval_success_listener, &params);
	progress_dlg.DoModal();

}

HBRUSH NewTokenDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_STATIC){
		if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_SETUP_LINK)->GetSafeHwnd()) {
			pDC->SetTextColor(RGB(200, 0, 200)); // make setup link purple
		}
		else if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_MANUAL_BADVALUE)->GetSafeHwnd()) {
			pDC->SetTextColor(RGB(200, 0, 0));
		}
		else if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_MANUAL_GOODTOKEN)->GetSafeHwnd()) {
			pDC->SetTextColor(RGB(0, 150, 0));
		}

		if (pWnd->GetDlgCtrlID() == IDC_STATICBTN_SETTINGS) {
			Gdiplus::Bitmap* settings_bitmap;
			HICON settings_icon;
			settings_bitmap = Gdiplus::Bitmap::FromFile(L"settings.png");
			settings_bitmap->GetHICON(&settings_icon);
			DrawIconEx(*pDC, 0, 0, settings_icon, 25, 25, 0, NULL, DI_NORMAL | DI_COMPAT);
			DestroyIcon(settings_icon);
		}
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return hbr;
}

void NewTokenDlg::iksm_token_respond(int nID_t, CString value_t) { // set response text for manual token retrieval
	GetDlgItem(nID_t)->SetWindowText(value_t);
	GetDlgItem(IDC_STATIC_MANUAL_BADVALUE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_MANUAL_GOODTOKEN)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_MANUAL_TESTING)->ShowWindow(SW_HIDE);

	GetDlgItem(nID_t)->ShowWindow(SW_SHOW);
}

void NewTokenDlg::OnEnChangeEditManual()
{
	CString text_iksm;
	edit_manual.GetWindowText(text_iksm);

	if (wcslen(text_iksm) < 40) {
		iksm_token_respond(IDC_STATIC_MANUAL_BADVALUE, _T("Invalid Token: Too short."));
	}
	else if (wcslen(text_iksm) > 40) {
		iksm_token_respond(IDC_STATIC_MANUAL_BADVALUE, _T("Invalid Token: Too long."));
	}
	else { // if the value has the correct size, we call the Nintendo server and try to get the user name
		iksm_token_respond(IDC_STATIC_MANUAL_TESTING, _T("Testing new token..."));
		string splatnet_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/records",
			static_cast<string>(CW2A(text_iksm.GetString())));
		if (splatnet_string.find("AUTHENTICATION_ERROR") == std::string::npos) { // if returned data doesn't contain "AUTHENTICATION ERROR"
			nlohmann::json j = nlohmann::json::parse(splatnet_string);
			string nickname = j["records"]["player"]["nickname"];
			CString welcome = _T("Valid Token! Welcome, ");
			welcome += CString(nickname.c_str());
			welcome += _T("!");
			iksm_token_respond(IDC_STATIC_MANUAL_GOODTOKEN, welcome);
			button_ok.EnableWindow(true);
			iksm_token = static_cast<string>(CW2A(text_iksm.GetString()));
		}
		else {
			iksm_token_respond(IDC_STATIC_MANUAL_BADVALUE, _T("Invalid Token: Failed to authenticate."));
		}
	}

	// TODO:  Add your control notification handler code here
}


void NewTokenDlg::OnBnClickedOk()
{
	if (iksm_token) { // if an iksm_token has been set manually
		ifstream in_file("settings.txt");
		nlohmann::json j;
		in_file >> j;
		in_file.close();

		ofstream out_file("settings.txt");
		CString cstring_temp;

		j["iksm-session"] = *iksm_token;

		out_file << j;

		out_file.close();
	}
	CDialogEx::OnOK();
}


void NewTokenDlg::OnStnClickedStaticSetupLink()
{
	SetupRetrievalDlg setup_dlg;
	setup_dlg.button_runiksm = &button_iksm_start;
	setup_dlg.DoModal();
}

void NewTokenDlg::OnStnClickedSettings() {
	CSettingsDialog settings_dlg;
	settings_dlg.DoModal();
}


void NewTokenDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnClose();
}


BOOL NewTokenDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_RETRIEVAL_SETUP)->GetSafeHwnd()) {
		SetCursor(cursor_hand);
		return true;
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}
