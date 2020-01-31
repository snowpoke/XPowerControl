#pragma once
#include "resource.h"
#include <afxdialogex.h>
#include <optional>

// SetupRetrievalDlg dialog

class SetupRetrievalDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SetupRetrievalDlg)

public:
	SetupRetrievalDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~SetupRetrievalDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETUP_AUTORETRIEVAL };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CEdit edit_mitm;
	CEdit edit_emu;
	CButton button_mitm;
	CButton button_emu;
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedOk();
	std::optional<CButton*> button_runiksm = {};
	afx_msg void OnEnChangeEditMitm();
	CButton button_ok;
	afx_msg void OnEnUpdateEditEmu();
	void check_okbutton();
	CEdit edit_emucmd;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnStnClickedStaticTestrun();
	HCURSOR cursor_hand;
	HCURSOR cursor_arrow;
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
