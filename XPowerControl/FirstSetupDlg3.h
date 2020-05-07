#pragma once
#include <afxdialogex.h>
#include "resource.h"
#include "logging.h"
#include <optional>
#include <string>

// FirstSetupDlg3 dialog

std::string get_local_ip();

class FirstSetupDlg3 : public CDialogEx
{
	DECLARE_DYNAMIC(FirstSetupDlg3)

public:
	// we need to carry some information from dialog to dialog
	FirstSetupDlg3(std::optional<std::string> nox_file_t,
		std::optional<std::string> mitm_file_t,
		CWnd* pParent = nullptr);
	virtual ~FirstSetupDlg3();

	std::optional<std::string> bs_file;
	std::optional<std::string> mitm_file;
	std::optional<HANDLE> bs_handle;
	std::optional<HANDLE> mitm_handle;
	CWinThread* winthread_installation_checker;
	bool stop_installation_checker = false;

	static UINT thread_installation_checker(LPVOID pParam);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTSETUP3 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonInstallNox();
	afx_msg void OnBnClickedButtonInstallMitm();
	afx_msg BOOL OnInitDialog();
	CButton button_ok;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
private:
	logging::log_ptr _logger;
};
