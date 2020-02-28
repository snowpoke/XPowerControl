// SetupRetrievalDlg.cpp : implementation file
//

#include "SetupRetrievalDlg.h"
#include "afxdialogex.h"
#include "afxdlgs.h"
#include "settings.h"
#include "wstring_transform.h"

using namespace std;


// SetupRetrievalDlg dialog

IMPLEMENT_DYNAMIC(SetupRetrievalDlg, CDialogEx)

SetupRetrievalDlg::SetupRetrievalDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETUP_AUTORETRIEVAL, pParent)
{

}

SetupRetrievalDlg::~SetupRetrievalDlg()
{
}

void SetupRetrievalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MITM, edit_mitm);
	DDX_Control(pDX, IDC_EDIT_EMU, edit_emu);
	DDX_Control(pDX, IDC_BUTTON3, button_mitm);
	DDX_Control(pDX, IDC_BUTTON4, button_emu);
	DDX_Control(pDX, IDOK, button_ok);
	DDX_Control(pDX, IDC_EDIT2, edit_emucmd);
}


BEGIN_MESSAGE_MAP(SetupRetrievalDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON3, &SetupRetrievalDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &SetupRetrievalDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDOK, &SetupRetrievalDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_MITM, &SetupRetrievalDlg::OnEnChangeEditMitm)
	ON_EN_UPDATE(IDC_EDIT_EMU, &SetupRetrievalDlg::OnEnUpdateEditEmu)
	ON_BN_CLICKED(IDC_BUTTON1, &SetupRetrievalDlg::OnBnClickedButton1)
	ON_STN_CLICKED(IDC_STATIC_TESTRUN, &SetupRetrievalDlg::OnStnClickedStaticTestrun)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// SetupRetrievalDlg message handlers

BOOL SetupRetrievalDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	// underline testrun link
	CFont* font_dlg = GetDlgItem(IDC_STATIC_TESTRUN)->GetFont();
	LOGFONT logfont_underline;

	font_dlg->GetLogFont(&logfont_underline);
	logfont_underline.lfUnderline = true;
	CFont* font_underline = new CFont();
	font_underline->CreateFontIndirectW(&logfont_underline);

	GetDlgItem(IDC_STATIC_TESTRUN)->SetFont(font_underline);

	// create cursors for testrun link
	cursor_hand = LoadCursor(NULL, IDC_HAND);
	cursor_arrow = LoadCursor(NULL, IDC_ARROW);

	// fill elements with settings
	edit_mitm.SetWindowTextW(transform::s2ws(read_from_settings<string>("mitm_exec")).c_str());
	edit_emu.SetWindowTextW(transform::s2ws(read_from_settings<string>("emu_exec")).c_str());
	edit_emucmd.SetWindowTextW(transform::s2ws(read_from_settings<string>("emu_cmd")).c_str());

	return TRUE;
}


void SetupRetrievalDlg::OnBnClickedButton3()
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

void SetupRetrievalDlg::OnBnClickedButton4()
{
	// pick MITM executable
	CFileDialog file_dlg(true, _T("exe"), _T("*.exe"), OFN_FILEMUSTEXIST, _T("Executables (*.exe)"));
	if (file_dlg.DoModal() == IDOK) {
		CString picked_file = file_dlg.GetPathName();
		picked_file.Replace(_T("\\"), _T("/"));
		edit_emu.SetWindowTextW(picked_file);
		edit_emu.Invalidate();
		edit_emucmd.SetWindowTextW(picked_file + L" -clone:nox -startPackage:com.nintendo.znca");
		edit_emucmd.Invalidate();
	}

	//
}


void SetupRetrievalDlg::OnBnClickedOk()
{
	ifstream in_file("settings.txt");
	nlohmann::json j;
	in_file >> j;
	in_file.close();

	ofstream out_file("settings.txt");
	CString cstring_mitm, cstring_emu, cstring_emucmd;

	edit_mitm.GetWindowTextW(cstring_mitm);
	j["mitm_exec"] = string(CW2A(cstring_mitm.GetString()));

	edit_emu.GetWindowTextW(cstring_emu);
	j["emu_exec"] = string(CW2A(cstring_emu.GetString()));

	edit_emucmd.GetWindowTextW(cstring_emucmd);
	j["emu_cmd"] = string(CW2A(cstring_emucmd.GetString()));

	out_file << j;

	out_file.close();

	// enable "Run IKSM retrieval" button in parent window, if one has been defined
	if (button_runiksm && cstring_mitm != _T("") && cstring_emucmd != _T("") )
		(*button_runiksm)->EnableWindow(true);


	CDialogEx::OnOK();
}


void SetupRetrievalDlg::OnEnChangeEditMitm()
{
	check_okbutton();
}


void SetupRetrievalDlg::OnEnUpdateEditEmu()
{
	check_okbutton();
}

void SetupRetrievalDlg::check_okbutton() {
	// enable OK button if both text boxes contain text
	CString cstring_mitm, cstring_emu;

	edit_mitm.GetWindowTextW(cstring_mitm);
	edit_emu.GetWindowTextW(cstring_emu);

	if (cstring_mitm != _T("") && cstring_emu != _T(""))
		button_ok.EnableWindow(true);
}

void SetupRetrievalDlg::OnBnClickedButton1()
{
	edit_emucmd.SetReadOnly(false);
}


void SetupRetrievalDlg::OnStnClickedStaticTestrun()
{
	confirm_and_run_script(this, edit_emucmd);
}


BOOL SetupRetrievalDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATIC_TESTRUN)->GetSafeHwnd()) {
		SetCursor(cursor_hand);
		return true;
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}
