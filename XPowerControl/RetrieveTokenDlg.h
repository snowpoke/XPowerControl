#pragma once
#include "resource.h"
#include <string>

// RetrieveTokenDlg dialog

class RetrieveTokenDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RetrieveTokenDlg)

public:
	RetrieveTokenDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~RetrieveTokenDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RETRIEVE_TOKEN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonMitm();
	CEdit tokenEdit;
	HANDLE mitmHandle;
	bool mitm_started = false;
	CButton start_mitm_button;
	std::wstring found_token;
	CButton ok_btn;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
	afx_msg void OnBnClickedButton1();
};
