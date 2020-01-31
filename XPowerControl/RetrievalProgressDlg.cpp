// RetrievalProgressDlg.cpp : implementation file
//

#include "RetrievalProgressDlg.h"
#include "afxdialogex.h"
#include "TokenRetriever.h"
#include "settings.h"
#include "wstring_transform.h"
#include "nlohmann/json.hpp"
#include <string>
#include <filesystem>
using namespace std;

// RetrievalProgressDlg dialog

IMPLEMENT_DYNAMIC(RetrievalProgressDlg, CDialogEx)

RetrievalProgressDlg::RetrievalProgressDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_RETRIEVE_PROGRESS, pParent)
{

}

RetrievalProgressDlg::~RetrievalProgressDlg()
{
}

void RetrievalProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_MAIN, progress);
}


BEGIN_MESSAGE_MAP(RetrievalProgressDlg, CDialogEx)
END_MESSAGE_MAP()


UINT progress_listener(LPVOID pParam) {
	RetrievalProgressDlg* dlg = (RetrievalProgressDlg*)pParam;

	// at the start of the launching process, we use the number of lines in process.txt to indicate how far we are
	int launching = true;

	do {
		// check the number of lines, update accordingly
		if (launching) {
			ifstream file("progress.txt");
			string line;
			int line_num = 0;
			while (getline(file, line))
				line_num++;
			dlg->progress.SetPos(min(line_num * 5,50)); // 5% per line, maximum 50%
		}

		if (dlg->progress.GetPos() != dlg->retriever.get_progress() && dlg->retriever.get_progress() != 0) {
			dlg->progress.SetPos(dlg->retriever.get_progress());
			launching = false;
		}


		Sleep(10);
	} while (dlg->progress.GetPos() < 100);

	dlg->GetDlgItem(IDC_STATIC_STATUS)->SetWindowTextW(L"Token has been retrieved successfully!");

	dlg->has_succeeded = true;
	if (!dlg->no_closing_after_success)
		dlg->SendMessage(WM_CLOSE);
	return 0;
}

UINT iksm_listener(LPVOID pParam) {
	RetrievalProgressDlg* dlg = (RetrievalProgressDlg*)pParam;

	while (true) {
		if (dlg->retriever.get_iksm_token().has_value()) {
			ifstream in_file("settings.txt");
			nlohmann::json j;
			in_file >> j;
			in_file.close();

			j["iksm-session"] = dlg->retriever.get_iksm_token().value();

			ofstream out_file("settings.txt");
			out_file << j;
			out_file.close();

			dlg->retriever.set_progress(100);

			break;
		}
		Sleep(10);
	}
	return 0;
}

// RetrievalProgressDlg message handlers

BOOL RetrievalProgressDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// make sure the window is in the foreground, even after helper software is called
	::SetForegroundWindow(this->GetSafeHwnd());
	::SetWindowPos(this->GetSafeHwnd(),
		HWND_TOPMOST,
		0,
		0,
		0,
		0,
		SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

	progress.SetRange(0, 100);
	progress.SetPos(0);

	// read mitm path and emulator command from settings
	string mitm_exec = read_from_settings<string>("mitm_exec");
	string emu_cmd = read_from_settings<string>("emu_cmd");


	retriever = TokenRetriever(GetDlgItem(IDC_STATIC_STATUS), {}, s2ws(mitm_exec), s2ws(emu_cmd));

	retriever.mitm_start();
	retriever.emu_start();
	
	// reset progress.txt
	if (filesystem::exists("progress.txt"))
		DeleteFile(L"progress.txt");

	// token_listener waits until mitm and emu have loaded, then loads new iksm token and stores it
	AfxBeginThread(retriever.token_listener, &retriever);

	// progress_listener listens to progress_percent on the iksm retriever, updates progress bar accordingly
	AfxBeginThread(progress_listener, this);

	// iksm_listener listens to iksm_token, and saves it to settings if found
	AfxBeginThread(iksm_listener, this);

	return TRUE;
}
