#pragma once
#include <afxdialogex.h>
#include <map>
#include <vector>
#include <functional>

#define MSG_UPDATE_DL_PROGRESS 1
#define MSG_STOP_LISTENER 2
#define MSG_NONE 99

class CustomDialogBase : public CDialogEx
{
public:
	CustomDialogBase(UINT nIDTemplate, CWnd* pParent=NULL);   // same syntax as CDialogEx constructor
	virtual ~CustomDialogBase() {};

	virtual afx_msg BOOL OnInitDialog();

	static UINT message_listener(LPVOID pParam);

	void send_message(UINT message_t);
	CWinThread* winthread_message_listener;
protected:
	UINT message = MSG_NONE;
	std::map<int, std::function<void(void* pParam)>> msg_to_function_map; // maps messges to functions that are called when that message is transmitted
	void register_message_mapping(int message_t, std::function<void(void* pParam)> function_t);
};

