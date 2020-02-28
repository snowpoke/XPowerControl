// FirstSetupDlg3.cpp : implementation file
//

#include "FirstSetupDlg2.h"
#include "FirstSetupDlg3.h"
#include "FirstSetupDlg4.h"
#include "TokenRetriever.h"
#include "afxdialogex.h"
#include "wstring_transform.h"
#include <string>
#include <optional>
#include <map>
#include <vector>
#include <algorithm>
#include "bit7zlibrary.hpp"
#include "bitextractor.hpp"
#include "bitformat.hpp"
#include "bitexception.hpp"
#include "FirstSetup.h"
#include "Version.h"
#include "ActionRequiredDlg.h"

typedef unsigned char byte_t;

using namespace std;
// FirstSetupDlg3 dialog

IMPLEMENT_DYNAMIC(FirstSetupDlg3, CDialogEx)

FirstSetupDlg3::FirstSetupDlg3(optional<string> bs_file_t,
	optional<string> mitm_file_t,
	CWnd* pParent) : CDialogEx(IDD_FIRSTSETUP3, pParent) {
	bs_file = bs_file_t;
	mitm_file = mitm_file_t;
}

FirstSetupDlg3::~FirstSetupDlg3()
{
}

void FirstSetupDlg3::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, button_ok);
}


UINT FirstSetupDlg3::thread_installation_checker(LPVOID pParam) {

	FirstSetupDlg3* dlg = (FirstSetupDlg3*)pParam;

	bool installed_bs = false;
	bool installed_mitm = false;
	bool transferred_image = false;
	bool block_ok = false;
	optional<Version> bs_version, mitm_version;
	CRegKey registry;
	ULONG sz_version = 20;
	while (!dlg->stop_installation_checker) {
		sz_version = 20;
		auto nox_key_ret = registry.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BlueStacks", KEY_READ);
		if (nox_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
			bs_version = Version();
			registry.QueryStringValue(L"DisplayVersion", bs_version->get_cstr_buffer(sz_version), &sz_version);
			bs_version->make_from_buffer();
		}

		registry.Close();

		sz_version = 20;
		auto mitm_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\mitmproxy 5.0.1", KEY_READ);
		if (mitm_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
			mitm_version = Version();
			registry.QueryStringValue(L"DisplayVersion", mitm_version->get_cstr_buffer(sz_version), &sz_version);
			mitm_version->make_from_buffer();
		}
		registry.Close();
		// if Nox is already installed we check the Nox version to see if we need to update
		if (bs_version) {
			Version required_version = Version::from_int(4,170,10,1001);

			installed_bs = (bs_version >= required_version);

			if (bs_version < required_version)
				dlg->GetDlgItem(IDC_STATIC_NOX)->SetWindowText(L"BlueStacks installation found, but needs to be updated to a later version.");
			else if (bs_version > required_version)
				dlg->GetDlgItem(IDC_STATIC_NOX)->SetWindowText(L"BlueStacks installation found, but is above the expected version. You may experience issues.");
			else if (bs_version == required_version)
				dlg->GetDlgItem(IDC_STATIC_NOX)->SetWindowText(L"BlueStacks installation has been found!");

		}

		// if mitmproxy is installed, we don't need to update in any case
		if (mitm_version)
			installed_mitm = true;

		if (installed_bs) {
			dlg->GetDlgItem(IDC_BUTTON_INSTALL_NOX)->EnableWindow(false);
			if (dlg->bs_handle) { // close nox if it has been installed through this program
				Sleep(300); // wait a bit for final files to transfer
				CloseHandle(dlg->bs_handle.value());
				dlg->bs_handle = optional<HANDLE>(); // reset optional to false so this doesn't get called again
			}
			if (dlg->bs_file && !transferred_image) { // if this program just got installed, transfer android system image
				dlg->GetDlgItem(IDC_STATIC_NOX)->SetWindowText(L"BlueStacks installation has been found - transferring data...");
				block_ok = true; // don't give access to ok button while this is running
				FirstSetup::setup_bs(dlg->GetDlgItem(IDC_STATIC_NOX));

				transferred_image = true;
				block_ok = false;
				dlg->GetDlgItem(IDC_STATIC_NOX)->SetWindowText(L"BlueStacks installation has been found - data transferred!");
			}
		}

		if (installed_mitm) {
			dlg->GetDlgItem(IDC_BUTTON_INSTALL_MITM)->EnableWindow(false);
			dlg->GetDlgItem(IDC_STATIC_MITM)->SetWindowText(L"Mitm installation has been found!");
			if (dlg->mitm_handle) { // close mitm if it has been installed through this program
				Sleep(800); // wait a bit for final files to transfer
				CloseHandle(dlg->mitm_handle.value());
				dlg->mitm_handle = optional<HANDLE>(); // reset handle so that this doens't get called again
				FirstSetup::setup_mitm();
			}
		}

		if (installed_bs && installed_mitm && !block_ok) {
			dlg->button_ok.EnableWindow(true);
		}

		Sleep(250);
	}

	return 0;
}



BEGIN_MESSAGE_MAP(FirstSetupDlg3, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_INSTALL_NOX, &FirstSetupDlg3::OnBnClickedButtonInstallNox)
	ON_BN_CLICKED(IDC_BUTTON_INSTALL_MITM, &FirstSetupDlg3::OnBnClickedButtonInstallMitm)
	ON_BN_CLICKED(IDOK, &FirstSetupDlg3::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &FirstSetupDlg3::OnBnClickedCancel)
END_MESSAGE_MAP()


// FirstSetupDlg3 message handlers
BOOL FirstSetupDlg3::OnInitDialog()
{
	CDialog::OnInitDialog();

	HICON m_hIcon1 = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon1, TRUE);			// Set big icon
	SetIcon(m_hIcon1, FALSE);		// Set small icon

	winthread_installation_checker = AfxBeginThread(thread_installation_checker, this);
	return 0;
}

void FirstSetupDlg3::OnBnClickedButtonInstallNox()
{
	bs_handle = TokenRetriever::run_command(transform::s2ws(*bs_file));
	Sleep(1000);
	ActionRequiredDlg("media/bluestacks_install_comp.png",
		L"Please press \"Install\" and don't close any windows that open.\nwoomyDX will perform several setup steps for you. ");

}


void FirstSetupDlg3::OnBnClickedButtonInstallMitm()
{
	mitm_handle = TokenRetriever::run_command(transform::s2ws(*mitm_file));
	Sleep(1000);
	ActionRequiredDlg("media/mitm_install_comp.png",
		L"Please go through the MITM setup.\nThe installer will quit automatically once a full installation has been found.");
}


void FirstSetupDlg3::OnBnClickedOk()
{
	// stop installation checker thread
	ShowWindow(SW_HIDE);
	stop_installation_checker = true;
	WaitForSingleObject(winthread_installation_checker->m_hThread, 1000);
	FirstSetupDlg4 dlg;
	dlg.DoModal();
	CDialogEx::OnOK();
}


void FirstSetupDlg3::OnBnClickedCancel()
{
	if (FirstSetup::confirm_exit(this->GetSafeHwnd())) {
		stop_installation_checker = true;
		WaitForSingleObject(winthread_installation_checker->m_hThread, 1000);
		FirstSetup::cleanup();
		CDialogEx::OnCancel();
		PostQuitMessage(0);
	}
}
