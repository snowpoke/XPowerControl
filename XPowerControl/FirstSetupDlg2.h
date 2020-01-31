#pragma once
#include "resource.h"
#include <afxdialogex.h>
#include <iostream>
#define MSG_UPDATE_DL_PROGRESS 1
#define MSG_STOP_LISTENER 2
#define MSG_NONE 99

// our implementation of the IBindStatusCallback interface
class FirstSetupDlg2;

class Callback : public IBindStatusCallback {
public:
	unsigned long progress = 0;
	unsigned long max_dl = 0;
	unsigned long status_code = 0;
	FirstSetupDlg2* dlg;

	Callback() : dlg(nullptr) {}
	Callback(FirstSetupDlg2* dlg_t) : dlg(dlg_t) {}

	HRESULT GetBindInfo(
		DWORD* grfBINDF,
		BINDINFO* pbindinfo
	);

	HRESULT GetPriority(
		long* pnPriority
	);

	HRESULT OnDataAvailable(
		DWORD     grfBSCF,
		DWORD     dwSize,
		FORMATETC* pformatetc,
		STGMEDIUM* pstgmed
	);

	HRESULT OnLowResource(
		DWORD dwReserved
	);

	HRESULT OnObjectAvailable(
		REFIID   riid,
		IUnknown* punk
	);

	HRESULT OnProgress(
		unsigned long ulProgress,
		unsigned long ulProgressMax,
		unsigned long ulStatusCode,
		LPCWSTR       szStatusText
	);

	HRESULT OnStartBinding(
		DWORD    dwReserved,
		IBinding* pib
	);

	HRESULT OnStopBinding(
		HRESULT hresult,
		LPCWSTR szError
	);

	HRESULT QueryInterface(
		REFIID riid,
		void** ppvObject
	);

	ULONG AddRef() { return 0; }

	ULONG Release() { return 0; }
};

// FirstSetupDlg2 dialog

class FirstSetupDlg2 : public CDialogEx
{
	DECLARE_DYNAMIC(FirstSetupDlg2)

public:
	FirstSetupDlg2(bool needs_nox_t, bool needs_mitm_t, CWnd* pParent = nullptr);   // standard constructor
	virtual ~FirstSetupDlg2();
	bool needs_nox;
	bool needs_mitm;
	virtual BOOL OnInitDialog();
	static UINT message_listener(LPVOID pParam);
	void on_update_dl_progress();
	void send_message(UINT message_t) { message = message_t; }
	Callback nox_callback, mitm_callback, vmachine_callback, nso_callback;
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTSETUP2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	UINT message = MSG_NONE;
public:
	CProgressCtrl progress_dl;
};
