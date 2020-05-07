#pragma once
#include "resource.h"
#include <afxdialogex.h>

// messages that can be sent to the dialog
#define MSG_NONE 0
#define MSG_REFRESH_SETTINGS 1
#define ST_ALL 1
#define ST_AUTHONLY 2

// CSettingsDialog dialog

class CSettingsDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDialog)

public:
	CSettingsDialog(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSettingsDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnEnChangeMfcmaskededit1();
	CEdit edit_iksm;
	CEdit edit_mitm;
	CEdit edit_emu;
	CEdit edit_datadir;
	CEdit edit_startcmd;
	CEdit edit_endcmd;
	CButton check_savematchdata;
	CButton check_runstartcmd;
	CButton check_runendcmd;
	afx_msg BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
	CButton button_iksm;
	CButton button_mitm;
	CButton button_emu;
	CButton button_filedir;
	CButton button_startcmd;
	CButton button_endcmd;
	afx_msg void OnClose();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedCheckSavematchdata();
	afx_msg void OnBnClickedCheckRunstartcmd();
	afx_msg void OnBnClickedCheckRunendcmd();
	CButton button_iksmfail;
	afx_msg void OnBnClickedButtonIksmFailscreen();
	static UINT message_listener(LPVOID pParam);
	void send_message(UINT message_t);
	void refresh_settings(UINT limit_performance = ST_ALL);
	UINT message = MSG_NONE;
	afx_msg void OnBnClickedButtonRetrievetoken();
	afx_msg void OnStnClickedStaticRetrievalSetup();
	CButton check_hidepower;
	CEdit edit_maxprecise;
	afx_msg void OnBnClickedDohidepower();
	CWinThread* handle_message_listener;
	afx_msg void OnBnClickedButton7();
	CEdit edit_nox;
	void save_settings(UINT limit_performance = ST_ALL);
	afx_msg void OnNcDestroy();
	bool kill_message_listener = false;
	CButton check_ontop;
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	HCURSOR cursor_hand;
	HCURSOR cursor_arrow;
	afx_msg void OnBnClickedTestNsoupdate();
};
