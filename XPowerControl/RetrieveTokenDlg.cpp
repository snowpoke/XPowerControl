// RetrieveTokenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RetrieveTokenDlg.h"
#include "afxdialogex.h"
#include "token_retrieve.h"
#include "nlohmann/json.hpp"
#include "monitor.h"
#include "rotation_monitor.h"
#include <fstream>
#include <string>

using namespace std;

// RetrieveTokenDlg dialog

IMPLEMENT_DYNAMIC(RetrieveTokenDlg, CDialogEx)

RetrieveTokenDlg::RetrieveTokenDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RETRIEVE_TOKEN, pParent)
{
}

RetrieveTokenDlg::~RetrieveTokenDlg()
{
}

BOOL RetrieveTokenDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	tokenEdit.SetWindowTextW(L"> Follow Step 1 to start process <");

	return 1;
}

void RetrieveTokenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOKEN_EDIT, tokenEdit);
	DDX_Control(pDX, IDC_BUTTON_MITM, start_mitm_button);
	DDX_Control(pDX, IDC_BUTTON1, ok_btn);
}


BEGIN_MESSAGE_MAP(RetrieveTokenDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MITM, &RetrieveTokenDlg::OnBnClickedButtonMitm)
	ON_BN_CLICKED(IDCANCEL, &RetrieveTokenDlg::OnBnClickedCancel)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON1, &RetrieveTokenDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// RetrieveTokenDlg message handlers


void RetrieveTokenDlg::OnBnClickedButtonMitm()
{
	mitmHandle = mitm_start();
	mitm_started = true;
	tokenEdit.SetWindowTextW(L"> Follow Step 2 <");
	start_mitm_button.EnableWindow(FALSE);
	AfxBeginThread(token_listener, this);
}


void RetrieveTokenDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}


void RetrieveTokenDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	if (mitm_started) // if mitm is running, exit it now
		TerminateProcess(mitmHandle, 0);

	CDialogEx::OnClose();
}


void RetrieveTokenDlg::OnBnClickedButton1()
{
	ifstream ifile("settings.txt");
	nlohmann::json j;
	ifile >> j;
	ifile.close();

	j["iksm-session"] = string(found_token.begin(), found_token.end());
	ofstream ofile("settings.txt");
	ofile << j;
	ofile.close();

	AfxBeginThread(monitor_main, AfxGetApp()->GetMainWnd());
	AfxBeginThread(monitor_rotation, AfxGetApp()->GetMainWnd());
	SendMessage(WM_CLOSE);
}
