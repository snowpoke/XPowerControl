// ActionRequiredDlg.cpp : implementation file
//

#include "ActionRequiredDlg.h"
#include "afxdialogex.h"
#include "wstring_transform.h"
#include <atlimage.h>
#include <string>
#include "PictureCtrl.h"

using namespace std;
// ActionRequiredDlg dialog

IMPLEMENT_DYNAMIC(ActionRequiredDlg, CDialogEx)

ActionRequiredDlg::ActionRequiredDlg(string picture_file_t, wstring description_text_t, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ACTION_REQUIRED, pParent)
{
	picture_file = picture_file_t;
	description_text = description_text_t;
	DoModal();
}

ActionRequiredDlg::~ActionRequiredDlg()
{
}

void ActionRequiredDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_SCREENSHOT, img_screenshot);
}


BEGIN_MESSAGE_MAP(ActionRequiredDlg, CDialogEx)
END_MESSAGE_MAP()


// ActionRequiredDlg message handlers

// FirstSetupDlg3 message handlers
BOOL ActionRequiredDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// set font for the dialog title
	CFont* font_dlg = GetDlgItem(IDC_STATIC_TITLE)->GetFont();
	LOGFONT logfont_bold;
	font_dlg->GetLogFont(&logfont_bold);
	logfont_bold.lfWeight = 700;
	logfont_bold.lfHeight = 14;
	CFont* font_bold = new CFont();
	font_bold->CreateFontIndirectW(&logfont_bold);
	GetDlgItem(IDC_STATIC_TITLE)->SetFont(font_bold);

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	// make sure the window is in the foreground, even after helper software is called
	::SetForegroundWindow(this->GetSafeHwnd());
	::SetWindowPos(this->GetSafeHwnd(),
		HWND_TOPMOST,
		0,
		0,
		0,
		0,
		SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

	img_screenshot.LoadFromFile(CString(transform::s2ws(picture_file).c_str()));

	GetDlgItem(IDC_STATIC_DESCRIPTION)->SetWindowTextW(description_text.c_str());


	return 0;
}
