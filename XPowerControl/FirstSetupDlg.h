#pragma once
#include "resource.h"
#include <afxdialogex.h>
#include <optional>

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
	afx_msg void OnBnClickedOk();
	bool needs_mitm = true;
	bool needs_nox = true;
};
