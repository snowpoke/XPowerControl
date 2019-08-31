
// XPowerControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XPowerControl.h"
#include "XPowerControlDlg.h"
#include "RetrieveTokenDlg.h"
#include "afxdialogex.h"
#include "monitor.h"
#include "rotation_monitor.h"
#include "http_requests.h"
#include "settings.h"
#include "analytics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CXPowerControlDlg dialog

using namespace std;

CXPowerControlDlg::CXPowerControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_XPOWERCONTROL_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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
END_MESSAGE_MAP()


// CXPowerControlDlg message handlers

BOOL CXPowerControlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

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

#ifdef DEBUG
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "This is the debugging window. If you're not the developer of this software and you can see this, please contact the developer and tell her that she's a dumb dumb.";
#endif


	// run game monitor

	string SESSID = read_from_settings("iksm-session");
	if (SESSID == "") {
		AfxMessageBox(_T("No value for iksm_session was found. "
			"Please check the description in README.txt to learn how to use this program. Press ENTER to exit."));
		AfxGetMainWnd()->SendMessage(WM_CLOSE);
	}

	// test run a call to chech whether the sessid is valid
	string schedule_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/schedules", SESSID);
	// if the string contains "AUTHENTICATION_ERROR", we quit
	if (schedule_json_string.find("AUTHENTICATION_ERROR") != std::string::npos) {
		AfxMessageBox(_T("Your IKSM token has run out. After pressing OK, you will be redirected to the token refresher tool. Check the README.txt file for details."));
		RetrieveTokenDlg dlg;
		dlg.DoModal();
		return FALSE;
	}
	
	thread_rotation_monitor = AfxBeginThread(monitor_rotation, this);
	thread_monitor_main = AfxBeginThread(monitor_main_alt, this);
	AfxBeginThread(upload_analytics, &analytics);
	return TRUE;  // return TRUE  unless you set the focus to a control
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
		CPen penGreen, penRed;
		CBrush brushGreen, brushRed;
		penGreen.CreatePen(PS_SOLID, 1, RGB(200,255,200));
		penRed.CreatePen(PS_SOLID, 1, RGB(255,200, 200));
		brushGreen.CreateSolidBrush(RGB(200,255,200));
		brushRed.CreateSolidBrush(RGB(255,200,200));

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
		/*if(pWnd->GetDlgCtrlID() == IDC_WINVALUE || pWnd->GetDlgCtrlID() == IDC_WINTEXT)
			pDC->SetBkColor(RGB(200, 255, 200));
		if (pWnd->GetDlgCtrlID() == IDC_LOSEVALUE || pWnd->GetDlgCtrlID() == IDC_LOSETEXT)
			pDC->SetBkColor(RGB(255, 200, 200));*/
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
	// TODO: Add your message handler code here and/or call default
	kill_monitor_main = true;
	kill_rotation_monitor = true;
	DWORD main_response = WaitForSingleObject(thread_monitor_main, 5000);
	DWORD rotation_response = WaitForSingleObject(thread_rotation_monitor, 5000);
	CDialogEx::OnClose();
}
