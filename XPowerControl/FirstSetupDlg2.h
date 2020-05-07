#pragma once
#include <afxdialogex.h>
#include "resource.h"
#include "http_requests.h"
#include "CustomDialogBase.h"
#include "logging.h"
#include <iostream>
#include <vector>
#include <array>

// our implementation of the IBindStatusCallback interface
class FirstSetupDlg2;

typedef unsigned char byte_t;
void write_binary_buffer(std::wstring path_t, std::vector<byte_t> buffer_t);

// FirstSetupDlg2 dialog

class FirstSetupDlg2 : public CustomDialogBase
{
	DECLARE_DYNAMIC(FirstSetupDlg2)

public:
	FirstSetupDlg2(bool needs_nox_t, bool needs_mitm_t, CWnd* pParent = nullptr);   // standard constructor
	virtual ~FirstSetupDlg2();
	bool needs_bs;
	bool needs_mitm;
	virtual BOOL OnInitDialog();
	static void on_update_dl_progress_wrapper(void* pParam);
	void on_update_dl_progress();
	http_requests::Callback bs_callback, mitm_callback, nso_callback;
	std::array<http_requests::Callback,4> vmachine_callback;
	std::array<http_requests::DownloadParam,4> vmachine_dl_param;
	std::wstring vmachine_dl_url;
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTSETUP2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl progress_dl;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
private:
	logging::log_ptr _logger;
};
