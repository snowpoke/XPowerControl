// CSettingsDialog.cpp : implementation file
//

#include "CSettingsDialog.h"
#include "NewTokenDlg.h"
#include "afxdialogex.h"
#include "afxdlgs.h"
#include "settings.h"
#include "wstring_transform.h"
#include "RetrievalProgressDlg.h"
#include "SetupRetrievalDlg.h"
#include <regex>

using namespace std;

// CSettingsDialog dialog

IMPLEMENT_DYNAMIC(CSettingsDialog, CDialogEx)

CSettingsDialog::CSettingsDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS_DIALOG, pParent)
{

}

CSettingsDialog::~CSettingsDialog()
{
}

void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_IKSM, edit_iksm);
	DDX_Control(pDX, IDC_EDIT_MITM, edit_mitm);
	DDX_Control(pDX, IDC_EDIT_EMU, edit_emu);
	DDX_Control(pDX, IDC_EDIT_DATADIR, edit_datadir);
	DDX_Control(pDX, IDC_EDIT_STARTCMD, edit_startcmd);
	DDX_Control(pDX, IDC_EDIT_ENDCMD, edit_endcmd);
	DDX_Control(pDX, IDC_CHECK_SAVEMATCHDATA, check_savematchdata);
	DDX_Control(pDX, IDC_CHECK_RUNSTARTCMD, check_runstartcmd);
	DDX_Control(pDX, IDC_CHECK_RUNENDCMD, check_runendcmd);
	DDX_Control(pDX, IDC_BUTTON1, button_iksm);
	DDX_Control(pDX, IDC_BUTTON3, button_mitm);
	DDX_Control(pDX, IDC_BUTTON4, button_emu);
	DDX_Control(pDX, IDC_BUTTON2, button_filedir);
	DDX_Control(pDX, IDC_BUTTON5, button_startcmd);
	DDX_Control(pDX, IDC_BUTTON6, button_endcmd);
	DDX_Control(pDX, IDC_BUTTON_IKSM_FAILSCREEN, button_iksmfail);
	DDX_Control(pDX, IDC_DOHIDEPOWER, check_hidepower);
	DDX_Control(pDX, IDC_EDIT_MAXPRECISE, edit_maxprecise);
	DDX_Control(pDX, IDC_EDIT_NOX, edit_nox);
	DDX_Control(pDX, IDC_CHECK_DOONTOP, check_ontop);
}


BEGIN_MESSAGE_MAP(CSettingsDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON2, &CSettingsDialog::OnBnClickedButton2)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CSettingsDialog::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CSettingsDialog::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON3, &CSettingsDialog::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CSettingsDialog::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CSettingsDialog::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CSettingsDialog::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_CHECK_SAVEMATCHDATA, &CSettingsDialog::OnBnClickedCheckSavematchdata)
	ON_BN_CLICKED(IDC_CHECK_RUNSTARTCMD, &CSettingsDialog::OnBnClickedCheckRunstartcmd)
	ON_BN_CLICKED(IDC_CHECK_RUNENDCMD, &CSettingsDialog::OnBnClickedCheckRunendcmd)
	ON_BN_CLICKED(IDC_BUTTON_IKSM_FAILSCREEN, &CSettingsDialog::OnBnClickedButtonIksmFailscreen)
	ON_BN_CLICKED(IDC_BUTTON_RETRIEVETOKEN, &CSettingsDialog::OnBnClickedButtonRetrievetoken)
	ON_STN_CLICKED(IDC_STATIC_RETRIEVAL_SETUP, &CSettingsDialog::OnStnClickedStaticRetrievalSetup)
	ON_BN_CLICKED(IDC_DOHIDEPOWER, &CSettingsDialog::OnBnClickedDohidepower)
	ON_BN_CLICKED(IDC_BUTTON7, &CSettingsDialog::OnBnClickedButton7)
	ON_WM_NCDESTROY()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CSettingsDialog message handlers


void CSettingsDialog::OnBnClickedCheck2()
{
	// TODO: Add your control notification handler code here
}


void CSettingsDialog::OnBnClickedButton2()
{
	// pick stream file folder
	CFolderPickerDialog folder_dlg;

	if (folder_dlg.DoModal() == IDOK) {
		CString picked_folder = folder_dlg.GetPathName();
		picked_folder.Replace(_T("\\"), _T("/"));
		picked_folder = picked_folder + _T("/");
		edit_datadir.SetWindowTextW(picked_folder);
		edit_datadir.Invalidate();
	}
}


void CSettingsDialog::OnEnChangeMfcmaskededit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void enable_by_checkbox(CDialog* dialog_t, CButton& checkbox_t, int element_id_t) {
	dialog_t->GetDlgItem(element_id_t)->EnableWindow(checkbox_t.GetCheck());
}


void CSettingsDialog::refresh_settings(UINT limit_performance /*=ST_ALL*/) {
	// fill elements with settings
	if (limit_performance == ST_ALL || limit_performance == ST_AUTHONLY) {
		edit_iksm.SetWindowTextW(s2ws(read_from_settings<string>("iksm-session")).c_str());
		edit_mitm.SetWindowTextW(s2ws(read_from_settings<string>("mitm_exec")).c_str());
		edit_nox.SetWindowTextW(s2ws(read_from_settings<string>("emu_exec")).c_str());
		edit_emu.SetWindowTextW(s2ws(read_from_settings<string>("emu_cmd")).c_str());
	}
	if (limit_performance == ST_ALL) {
		edit_datadir.SetWindowTextW(s2ws(read_from_settings<string>("matchdata_directory")).c_str());
		edit_startcmd.SetWindowTextW(s2ws(read_from_settings<string>("start_cmd")).c_str());
		edit_endcmd.SetWindowTextW(s2ws(read_from_settings<string>("end_cmd")).c_str());
		edit_maxprecise.SetWindowTextW(to_wstring(read_from_settings<int>("max_precise")).c_str());
		check_savematchdata.SetCheck(read_from_settings<bool>("do_savematchdata"));
		check_runstartcmd.SetCheck(read_from_settings<bool>("do_startcmd"));
		check_runendcmd.SetCheck(read_from_settings<bool>("do_endcmd"));
		check_hidepower.SetCheck(read_from_settings<bool>("do_hidepower"));
		check_ontop.SetCheck(read_from_settings<bool>("do_ontop"));
	}
	//  disable elements where features are disabled
	enable_by_checkbox(this, check_savematchdata, IDC_STATIC_DATADIR);
	enable_by_checkbox(this, check_savematchdata, IDC_EDIT_DATADIR);
	enable_by_checkbox(this, check_savematchdata, IDC_BUTTON2);

	enable_by_checkbox(this, check_runstartcmd, IDC_STATIC_STARTCMD);
	enable_by_checkbox(this, check_runstartcmd, IDC_EDIT_STARTCMD);
	enable_by_checkbox(this, check_runstartcmd, IDC_BUTTON5);

	enable_by_checkbox(this, check_runendcmd, IDC_STATIC_ENDCMD);
	enable_by_checkbox(this, check_runendcmd, IDC_EDIT_ENDCMD);
	enable_by_checkbox(this, check_runendcmd, IDC_BUTTON6);

	enable_by_checkbox(this, check_hidepower, IDC_STATIC_MAXPRECISE);
	enable_by_checkbox(this, check_hidepower, IDC_EDIT_MAXPRECISE);
}

BOOL CSettingsDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	refresh_settings();

	// underline retrieval setup link
	CFont* font_dlg = GetDlgItem(IDC_STATIC_RETRIEVAL_SETUP)->GetFont();
	LOGFONT logfont_underline;

	font_dlg->GetLogFont(&logfont_underline);
	logfont_underline.lfUnderline = true;
	CFont* font_underline = new CFont();
	font_underline->CreateFontIndirectW(&logfont_underline);

	GetDlgItem(IDC_STATIC_RETRIEVAL_SETUP)->SetFont(font_underline);

	// create cursors for retrieval setup link
	cursor_hand = LoadCursor(NULL, IDC_HAND);
	cursor_arrow = LoadCursor(NULL, IDC_ARROW);

	handle_message_listener = AfxBeginThread(message_listener, this);

	return true;
}

void CSettingsDialog::save_settings(UINT limit_performance /*=ST_ALL*/) {
	ifstream in_file("settings.txt");
	nlohmann::json j;
	in_file >> j;
	in_file.close();

	ofstream out_file("settings.txt");
	CString cstring_temp;

	if (limit_performance == ST_ALL || limit_performance == ST_AUTHONLY) {
		edit_iksm.GetWindowTextW(cstring_temp);
		j["iksm-session"] = string(CW2A(cstring_temp.GetString()));

		edit_mitm.GetWindowTextW(cstring_temp);
		j["mitm_exec"] = string(CW2A(cstring_temp.GetString()));

		edit_nox.GetWindowTextW(cstring_temp);
		j["emu_exec"] = string(CW2A(cstring_temp.GetString()));

		edit_emu.GetWindowTextW(cstring_temp);
		j["emu_cmd"] = string(CW2A(cstring_temp.GetString()));
	}
	if (limit_performance == ST_ALL) {
		edit_datadir.GetWindowTextW(cstring_temp);
		j["matchdata_directory"] = string(CW2A(cstring_temp.GetString()));

		edit_startcmd.GetWindowTextW(cstring_temp);
		j["start_cmd"] = string(CW2A(cstring_temp.GetString()));

		edit_endcmd.GetWindowTextW(cstring_temp);
		j["end_cmd"] = string(CW2A(cstring_temp.GetString()));

		edit_maxprecise.GetWindowTextW(cstring_temp);
		if (string(CW2A(cstring_temp.GetString())) == "") // if the max spread field is left empty, it is defaulted to 40
			j["max_precise"] = 40;
		else
			j["max_precise"] = stoi(string(CW2A(cstring_temp.GetString())));

		j["do_savematchdata"] = (check_savematchdata.GetCheck() == 1);
		j["do_startcmd"] = (check_runstartcmd.GetCheck() == 1);
		j["do_endcmd"] = (check_runendcmd.GetCheck() == 1);
		j["do_hidepower"] = (check_hidepower.GetCheck() == 1);
		j["do_ontop"] = (check_ontop.GetCheck() == 1);
	}
	out_file << j;

	out_file.close();
}

void CSettingsDialog::OnBnClickedOk()
{
	save_settings();
	CDialogEx::OnOK();
}

void CSettingsDialog::send_message(UINT message_t) {
	message = message_t;
}

UINT CSettingsDialog::message_listener(LPVOID pParam) {
	CSettingsDialog* dlg_settings = (CSettingsDialog*) pParam;
	while (!dlg_settings->kill_message_listener) {
		switch (dlg_settings->message) {
		case MSG_REFRESH_SETTINGS:
			dlg_settings->refresh_settings();
			dlg_settings->message = MSG_NONE;
			break;
		case MSG_NONE:
		default:
			break;
		}
		Sleep(1);
	}

	return 0;
}

void CSettingsDialog::OnBnClickedButton1()
{
	// Show/Hide iksm token
	if (edit_iksm.GetPasswordChar() == 0) { // hide if code visible
		edit_iksm.SetPasswordChar(L'●');
		edit_iksm.Invalidate();
		button_iksm.SetWindowTextW(L"Show");
		button_iksm.Invalidate();
	}
	else {
		edit_iksm.SetPasswordChar(0);
		edit_iksm.Invalidate();
		button_iksm.SetWindowTextW(L"Hide");
		button_iksm.Invalidate();
	}
}


void CSettingsDialog::OnBnClickedButton3()
{
	// pick MITM executable
	CFileDialog file_dlg(true, _T("exe"), _T("*.exe"), OFN_FILEMUSTEXIST, _T("Executables (*.exe)"));
	if (file_dlg.DoModal() == IDOK) {
		CString picked_file = file_dlg.GetPathName();
		picked_file.Replace(_T("\\"), _T("/"));
		edit_mitm.SetWindowTextW(picked_file);
		edit_mitm.Invalidate();
	}

}


static void run_script(CString command_t) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(NULL,
		&wstring(command_t)[0],
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

static void confirm_and_run_script(CDialog* dialog, CEdit& edit) {
	CString command;
	edit.GetWindowTextW(command);

	int message_confirm = MessageBox(dialog->GetSafeHwnd(), (LPCWSTR)L"This will run the command \"" + command + "\".\nContinue?",
		(LPCWSTR)L"Command Execution",
		MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3);

	if (message_confirm == IDYES) {
		run_script(command);
	}
}

void CSettingsDialog::OnBnClickedButton4()
{
	confirm_and_run_script(this, edit_emu);
}


void CSettingsDialog::OnBnClickedButton5()
{
	confirm_and_run_script(this, edit_startcmd);
}


void CSettingsDialog::OnBnClickedButton6()
{
	confirm_and_run_script(this, edit_endcmd);
}


void CSettingsDialog::OnBnClickedCheckSavematchdata()
{
	enable_by_checkbox(this, check_savematchdata, IDC_STATIC_DATADIR);
	enable_by_checkbox(this, check_savematchdata, IDC_EDIT_DATADIR);
	enable_by_checkbox(this, check_savematchdata, IDC_BUTTON2);
}


void CSettingsDialog::OnBnClickedCheckRunstartcmd()
{
	enable_by_checkbox(this, check_runstartcmd, IDC_STATIC_STARTCMD);
	enable_by_checkbox(this, check_runstartcmd, IDC_EDIT_STARTCMD);
	enable_by_checkbox(this, check_runstartcmd, IDC_BUTTON5);
}


void CSettingsDialog::OnBnClickedCheckRunendcmd()
{
	enable_by_checkbox(this, check_runendcmd, IDC_STATIC_ENDCMD);
	enable_by_checkbox(this, check_runendcmd, IDC_EDIT_ENDCMD);
	enable_by_checkbox(this, check_runendcmd, IDC_BUTTON6);
}

void CSettingsDialog::OnBnClickedDohidepower()
{
	enable_by_checkbox(this, check_hidepower, IDC_STATIC_MAXPRECISE);
	enable_by_checkbox(this, check_hidepower, IDC_EDIT_MAXPRECISE);
}


void CSettingsDialog::OnBnClickedButtonIksmFailscreen()
{
	// TODO: Add your control notification handler code here
	NewTokenDlg token_dlg(this);
	token_dlg.DoModal();
}


void CSettingsDialog::OnBnClickedButtonRetrievetoken()
{
	// check if emulator input fields have been changed, but not saved
	string settings_mitm_path = read_from_settings<string>("mitm_exec");
	string settings_emu_cmd = read_from_settings<string>("emu_cmd");

	CString cstring_temp;
	edit_mitm.GetWindowTextW(cstring_temp);
	string dlg_mitm_path = string(CW2A(cstring_temp.GetString()));

	edit_emu.GetWindowTextW(cstring_temp);
	string dlg_emu_cmd = string(CW2A(cstring_temp.GetString()));

	if ((settings_mitm_path != dlg_mitm_path) || (settings_emu_cmd != dlg_emu_cmd)) {
		int msgbox_id = MessageBox(
			(LPCWSTR)L"This will save your authentification settings before running the token retrieval. Do you want to proceed?",
			(LPCWSTR)L"Save Authentification Settings",
			MB_ICONQUESTION | MB_YESNO
		);

		switch (msgbox_id) {
		case IDYES: // if the user chooses yes, save mitm_exec and emu_cmd settings
			save_settings(ST_AUTHONLY);
			break;
		case IDNO:
			return; // if the user chooses no, we abort
		}
	}

	RetrievalProgressDlg progress_dlg;
	progress_dlg.DoModal();
	refresh_settings(ST_AUTHONLY);
}


void CSettingsDialog::OnStnClickedStaticRetrievalSetup()
{
	SetupRetrievalDlg setup_retrieval_dlg;
	setup_retrieval_dlg.DoModal();
	refresh_settings();
}

void CSettingsDialog::OnClose()
{
	CDialogEx::OnClose();
}


void CSettingsDialog::OnBnClickedButton7()
{
	// pick MITM executable
	CFileDialog file_dlg(true, _T("exe"), _T("*.exe"), OFN_FILEMUSTEXIST, _T("Executables (*.exe)"));
	if (file_dlg.DoModal() == IDOK) {
		CString picked_file = file_dlg.GetPathName();
		picked_file.Replace(_T("\\"), _T("/"));
		edit_nox.SetWindowTextW(picked_file);
		edit_nox.Invalidate();
		edit_emu.SetWindowTextW(picked_file + L" -clone:nox -startPackage:com.nintendo.znca");
		edit_emu.Invalidate();
	}
}


void CSettingsDialog::OnNcDestroy()
{
	kill_message_listener = true;
	DWORD main_response = WaitForSingleObject(handle_message_listener, 5000);
	CDialogEx::OnNcDestroy();
}


BOOL CSettingsDialog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_RETRIEVAL_SETUP)->GetSafeHwnd()) {
		SetCursor(cursor_hand);
		return true;
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}
