#pragma once
#include "CustomDialogBase.h"
#include "resource.h"
#include "Version.h"
#include "http_requests.h"

// UpdateNSODlg dialog

class UpdateNSODlg : public CustomDialogBase
{
	DECLARE_DYNAMIC(UpdateNSODlg)

public:
	UpdateNSODlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~UpdateNSODlg();
	static bool is_update_required();
	static bool is_update_required(Version& curr_version_t, Version& latest_version_t);
	static void on_update_dl_progress_wrapper(void* pParam);
	void on_update_dl_progress();
	afx_msg BOOL OnInitDialog();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UPDATE_NSO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	http_requests::Callback dl_callback;
	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl progressbar_nso;

	// parameters to be passed to the download thread, we don't want them to be deleted early
	http_requests::DownloadParam dl_param;
	wchar_t* dl_url;
	afx_msg void OnClose();
	Version latest_version, curr_version;
};
