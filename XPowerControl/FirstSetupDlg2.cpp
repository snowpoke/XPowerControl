// FirstSetupDlg2.cpp : implementation file
//

#include "FirstSetupDlg2.h"
#include "afxdialogex.h"
#include "wstring_transform.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <boost/format.hpp>
#pragma comment(lib, "urlmon.lib")

#define MITM_URL L"https://snapshots.mitmproxy.org/5.0.1/mitmproxy-5.0.1-windows-installer.exe"
#define NOX_URL L"http://res06.bignox.com/full/20191226/d27768ac0b314a8285d528ed82bd00eb.exe?filename=nox_setup_v6.6.0.0_full_intl.exe"
#define VMACHINE_URL L""
#define NSO_URL L"https://www.apkmirror.com/wp-content/themes/APKMirror/download.php?id=901009"


using namespace std;


// our implementation of the IBindStatusCallback interface

	HRESULT Callback::GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) {
		cout << "Called GetBindInfo";

		return S_OK;
	}

	HRESULT Callback::GetPriority(long* pnPriority) {
		cout << "Called GetPriority";

		return S_OK;
	}

	HRESULT Callback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) {
		cout << "Called OnDataAvailable";

		return S_OK;
	}

	HRESULT Callback::OnLowResource(DWORD dwReserved) {
		cout << "Called OnLowResource";

		return S_OK;
	}

	HRESULT Callback::OnObjectAvailable(REFIID riid, IUnknown* punk) {
		cout << "Called OnObjectAvailable";

		return S_OK;
	}

	HRESULT Callback::OnProgress(unsigned long ulProgress, unsigned long ulProgressMax,
		unsigned long ulStatusCode, LPCWSTR szStatusText) {
		cout << "Called OnProgress\n"
			<< "\tulProgress: " << ulProgress << "\n"
			<< "\tulProgressMax: " << ulProgressMax << "\n"
			<< "\tulStatusCode: " << ulStatusCode << "\n"
			<< "\tszStatusText" << szStatusText;

		progress = ulProgress;
		max_dl = ulProgressMax;
		status_code = ulStatusCode;
		dlg->send_message(MSG_UPDATE_DL_PROGRESS);

		return S_OK;
	}

	HRESULT Callback::OnStartBinding(DWORD dwReserved, IBinding* pib) {
		cout << "Called OnStartBinding";

		return S_OK;
	}

	HRESULT Callback::OnStopBinding(HRESULT hresult, LPCWSTR szError) {
		cout << "Called OnStopBinding";

		return S_OK;
	}

	HRESULT Callback::QueryInterface(REFIID riid, void** ppvObject) {
		return S_OK;
	}

// FirstSetupDlg2 dialog

IMPLEMENT_DYNAMIC(FirstSetupDlg2, CDialogEx)

FirstSetupDlg2::FirstSetupDlg2(bool needs_nox_t, bool needs_mitm_t, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FIRSTSETUP2, pParent)
{
	needs_nox = needs_nox_t;
	needs_mitm = needs_mitm_t;
}

FirstSetupDlg2::~FirstSetupDlg2()
{
}

void FirstSetupDlg2::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_DL, progress_dl);
}


UINT FirstSetupDlg2::message_listener(LPVOID pParam) {
	FirstSetupDlg2* dlg = (FirstSetupDlg2*)pParam;
	while (dlg->message != MSG_STOP_LISTENER) {
		switch (dlg->message) {
		case MSG_UPDATE_DL_PROGRESS:
			dlg->on_update_dl_progress();
			dlg->message = MSG_NONE;
			break;

		}
		Sleep(1);
	}

	return 0;
}

BEGIN_MESSAGE_MAP(FirstSetupDlg2, CDialogEx)
END_MESSAGE_MAP()

struct DownloadParam {
	wchar_t* dl_URL;
	wstring savepath_t;
	Callback* callback_t;
};

UINT thread_download_file(LPVOID pParam) {
	DownloadParam* dl_param = (DownloadParam*)pParam;

	URLDownloadToFile(NULL,
		dl_param->dl_URL,
		(dl_param->savepath_t).c_str(),
		0,
		dl_param->callback_t);

	return 0;
}

// FirstSetupDlg2 message handlers
BOOL FirstSetupDlg2::OnInitDialog() {
	CDialogEx::OnInitDialog();

	wstring savepath = L"./";

	mitm_callback = Callback(this);
	nox_callback = Callback(this);
	vmachine_callback = Callback(this);
	nso_callback = Callback(this);

	AfxBeginThread(message_listener, this);

	// load mitmproxy if neccessary
	if (needs_mitm) {
		AfxBeginThread(thread_download_file, new DownloadParam{ MITM_URL, L"installer_mitmproxy.exe", &mitm_callback });
	}
	else {
		GetDlgItem(IDC_STATIC_MITM_PROGRESS)->SetWindowText(L"Installed");
	}

	// load nox if neccessary
	if (needs_nox) {
		AfxBeginThread(thread_download_file, new DownloadParam{ NOX_URL, L"installer_nox.exe", &nox_callback });
	}
	else {
		GetDlgItem(IDC_STATIC_NOX_PROGRESS)->SetWindowText(L"Installed");
	}

	// always download virtual machine and nso app
	AfxBeginThread(thread_download_file, new DownloadParam{ VMACHINE_URL, L"vmachine.7z", &vmachine_callback });
	AfxBeginThread(thread_download_file, new DownloadParam{ NSO_URL, L"nintendo_switch_online.apk", &nso_callback });

	return true;
}

void FirstSetupDlg2::on_update_dl_progress() {
	long dl_progress_sum = mitm_callback.progress + nox_callback.progress + vmachine_callback.progress + nso_callback.progress;
	long dl_max_sum = mitm_callback.max_dl + nox_callback.max_dl + vmachine_callback.max_dl + nso_callback.max_dl;

	// status bar
	if (dl_max_sum > 0) {
		int pos = floor(static_cast<long double>(dl_progress_sum) * 100.0 / static_cast<long double>(dl_max_sum));
		progress_dl.SetPos(pos);
	}

	// status text
	boost::format mb_format("%.1fMB");
	string dl_progress_mb = (mb_format % (static_cast<long double>(dl_progress_sum) / 1000000.0)).str();
	string dl_max_mb = (mb_format % (static_cast<long double>(dl_max_sum) / 1000000.0)).str();
	wstring status_message = L"Loaded " + s2ws(dl_progress_mb) + L" out of " + s2ws(dl_max_mb) + L".";
	GetDlgItem(IDC_STATIC_DL)->SetWindowText(status_message.c_str());

	// status per download
	if (needs_nox) {
		string nox_progress_mb = (mb_format % (static_cast<long double>(nox_callback.progress) / 1000000.0)).str();
		string nox_max_mb = (mb_format % (static_cast<long double>(nox_callback.max_dl) / 1000000.0)).str();
		status_message = s2ws(nox_progress_mb) + L" / " + s2ws(nox_max_mb);
		GetDlgItem(IDC_STATIC_NOX_PROGRESS)->SetWindowText(status_message.c_str());
	}

	if (needs_mitm) {
		string mitm_progress_mb = (mb_format % (static_cast<long double>(mitm_callback.progress) / 1000000.0)).str();
		string mitm_max_mb = (mb_format % (static_cast<long double>(mitm_callback.max_dl) / 1000000.0)).str();
		status_message = s2ws(mitm_progress_mb) + L" / " + s2ws(mitm_max_mb);
		GetDlgItem(IDC_STATIC_MITM_PROGRESS)->SetWindowText(status_message.c_str());
	}

	string vmachine_progress_mb = (mb_format % (static_cast<long double>(vmachine_callback.progress) / 1000000.0)).str();
	string vmachine_max_mb = (mb_format % (static_cast<long double>(vmachine_callback.max_dl) / 1000000.0)).str();
	status_message = s2ws(vmachine_progress_mb) + L" / " + s2ws(vmachine_max_mb);
	GetDlgItem(IDC_STATIC_VMACHINE_PROGRESS)->SetWindowText(status_message.c_str());

	string nso_progress_mb = (mb_format % (static_cast<long double>(nso_callback.progress) / 1000000.0)).str();
	string nso_max_mb = (mb_format % (static_cast<long double>(nso_callback.max_dl) / 1000000.0)).str();
	status_message = s2ws(nso_progress_mb) + L" / " + s2ws(nso_max_mb);
	GetDlgItem(IDC_STATIC_NSO_PROGRESS)->SetWindowText(status_message.c_str());
}
