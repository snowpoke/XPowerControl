#pragma once
#include "resource.h"
#include "CSettingsDialog.h"
#include <afxdialogex.h>
#include <string>
#include <optional>
#define INFO_NOIKSM 1
#define INFO_EXPIRED 2
#define INFO_OK 3
#define INFO_FAIL 4

// NewTokenDlg dialog

class NewTokenDlg : public CDialogEx
{
	DECLARE_DYNAMIC(NewTokenDlg)

public:
	NewTokenDlg(UINT token_info, CWnd* pParent = nullptr);   // standard constructor
	NewTokenDlg(CSettingsDialog* dlg_t, UINT token_info = INFO_EXPIRED, CWnd* pParent = nullptr);
	virtual ~NewTokenDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NEW_TOKEN };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	CButton button_iksm_start;
	afx_msg void OnBnClickedButtonStart();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnEnChangeEditManual();
	void iksm_token_respond(int nID_t, CString value_t);
	CEdit edit_manual;
	CButton button_ok;
	std::optional<std::string> iksm_token;
	afx_msg void OnBnClickedOk();
	afx_msg void OnStnClickedStaticSetupLink();
	afx_msg void OnStnClickedSettings();
	CBitmap bitmap_settings;
	CButton button_settings;
	std::optional<CSettingsDialog*> settings_dlg;
	afx_msg void OnClose();
	UINT token_info;
	ULONG_PTR gdiplusToken; // required for GDI+
	CStatic staticbtn_settings;
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	HCURSOR cursor_hand;
	HCURSOR cursor_arrow;
};
