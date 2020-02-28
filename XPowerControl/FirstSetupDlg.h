#pragma once
#include "resource.h"
#include <afxdialogex.h>
#include <optional>
#include "Version.h"

// FirstSetupDlg dialog

class FirstSetupDlg : public CDialogEx
{
	DECLARE_DYNAMIC(FirstSetupDlg)

public:
	FirstSetupDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~FirstSetupDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTSETUP };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	Version this_ver, latest_ver;
	afx_msg void OnBnClickedOk();
	bool needs_mitm = true;
	bool needs_bs = true;
	afx_msg void OnBnClickedCancel();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	bool kill_update_info = false;
	CWinThread* update_info_monitor;
	afx_msg void OnStnClickedStaticInsufficientStorage();
	afx_msg void OnBnClickedButton1();
};
