#pragma once
#include "resource.h"
#include "TokenRetriever.h"
#include <afxdialogex.h>
#include <optional>


// RetrievalProgressDlg dialog

class RetrievalProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RetrievalProgressDlg)

public:
	RetrievalProgressDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~RetrievalProgressDlg();
	virtual BOOL OnInitDialog();
	bool no_closing_after_success; // if this is set to true, the dialog will not close on its own after finishing
	bool has_succeeded = false; // is true when iksm token is retrieved and saved
	CProgressCtrl progress;
	TokenRetriever retriever;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RETRIEVE_PROGRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
	CEdit edit_test;
};
