#pragma once
#include "resource.h"
#include "PictureCtrl.h"
#include <afxdialogex.h>
#include <string>

// ActionRequiredDlg dialog

class ActionRequiredDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ActionRequiredDlg)

public:
	ActionRequiredDlg(std::string picture_file_t, std::wstring description_text_t, CWnd* pParent = nullptr);
	virtual ~ActionRequiredDlg();
	std::string picture_file;
	std::wstring description_text;
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ACTION_REQUIRED };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedStaticPicture();
	afx_msg BOOL OnInitDialog();
	CStatic picture_description;
	CPictureCtrl img_screenshot;
};
