
// XPowerControlDlg.h : header file
//

#pragma once
#include <string>
#include "Analytics.h"

// CXPowerControlDlg dialog
class CXPowerControlDlg : public CDialogEx
{
// Construction
public:
	CXPowerControlDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_XPOWERCONTROL_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBnClickedButton1();
	bool match_running = false;
	bool update_rotation = false;
	bool kill_monitor_main = false; // flag whether monitor_main should be stopped
	bool kill_rotation_monitor = false;
	bool kill_thread_analytics = false;
	bool save_recent_match = false;
	CWinThread *thread_monitor_main, *thread_rotation_monitor;
	afx_msg void OnStnClickedLosevalue();
//	afx_msg void OnAcnStartAnimate1();
	CStatic m_loading_info;
	CStatic m_battle_start_text;
	Analytics analytics;
	afx_msg void OnClose();
};
