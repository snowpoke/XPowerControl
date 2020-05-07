#pragma once
#include <afxdialogex.h>
#include "resource.h"
#include "logging.h"
#include <string>


// FirstSetupDlg4 dialog
std::string get_local_ip();

class FirstSetupDlg4 : public CDialogEx
{
	DECLARE_DYNAMIC(FirstSetupDlg4)

public:
	FirstSetupDlg4(CWnd* pParent = nullptr);   // standard constructor
	virtual ~FirstSetupDlg4();
	afx_msg BOOL OnInitDialog();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTSETUP4 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl progress_prepare;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedCancel();
private:
	logging::log_ptr _logger;
};
