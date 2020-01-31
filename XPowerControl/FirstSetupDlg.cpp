// FirstSetupDlg.cpp : implementation file
//

#include "FirstSetupDlg.h"
#include "FirstSetupDlg2.h"
#include "afxdialogex.h"
#include <assert.h>
#include <string>
#include <optional>
#include <boost/algorithm/string/erase.hpp>

#define REGISTRY_PATH  L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

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
END_MESSAGE_MAP()


// FirstSetupDlg message handlers

BOOL FirstSetupDlg::OnInitDialog()
{
	// check if nox and mitmproxy are already installed
	CRegKey registry;
	optional<CString> nox_version;
	optional<CString> mitm_version;
	// open registry key
	ULONG sz_version = 20;
	auto nox_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Nox", KEY_READ);
	if (nox_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		nox_version = CString();
		registry.QueryStringValue(L"DisplayVersion", (*nox_version).GetBuffer(sz_version), &sz_version);
	}
	
	auto mitm_key_ret = registry.Open(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\mitmproxy 5.0.1", KEY_READ);
	if (mitm_key_ret == ERROR_SUCCESS) { // this returns ERROR_FILE_NOT_FOUND if the program doesn't exist
		mitm_version = CString();
		registry.QueryStringValue(L"DisplayVersion", (*mitm_version).GetBuffer(sz_version), &sz_version);
	}

	if (nox_version || mitm_version) {
		GetDlgItem(IDC_STATIC_NOTE)->ShowWindow(SW_SHOW);
	}
	
	// if Nox is already installed we check the Nox version to see if we need to update
	if (nox_version) {

		// we remove periods from the string and check if the resulting number is equal or higher than 6601 (for version 6.6.0.1)
		string version_clean = "";

		for (wchar_t c : wstring(*nox_version)) {
			if (c != '.')
				version_clean += c;
		}

		if (stoi(version_clean) >= 6600) // we don't need to update nox since the version is high enough
			needs_nox = false;
	}
	
	// if mitmproxy is installed, we don't need to update in any case
	if (mitm_version)
		needs_mitm = false;

	return true;
}

void FirstSetupDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	ShowWindow(SW_HIDE);
	FirstSetupDlg2 dlg(needs_nox, needs_mitm);
	dlg.DoModal();
	CDialogEx::OnOK();
}
