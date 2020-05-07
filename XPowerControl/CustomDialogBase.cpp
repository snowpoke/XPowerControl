#include <afxdialogex.h>
#include "CustomDialogBase.h"

CustomDialogBase::CustomDialogBase(UINT nIDTemplate, CWnd* pParent) : CDialogEx(nIDTemplate, pParent)
{
}


BOOL CustomDialogBase::OnInitDialog(){
	auto ret = CDialogEx::OnInitDialog();
	winthread_message_listener = AfxBeginThread(message_listener, this);

	return ret;
}

void CustomDialogBase::send_message(UINT message_t){ // TODO: Mutexes would be nice here
	message = message_t;
};

UINT CustomDialogBase::message_listener(LPVOID pParam) {
	CustomDialogBase* dlg = (CustomDialogBase*)pParam;
	while (dlg->message != MSG_STOP_LISTENER) {
		for (auto m : dlg->msg_to_function_map) { // check through all mapped messages
			if (dlg->message == m.first) // m.first is message to be checked, m.second is the function to be called
				m.second(pParam);
			if (dlg->message == m.first) // we double check for m.first here incase m.second sets a message
				dlg->message = MSG_NONE; // reset message so that function doesn't get called multiple times
		}
		
		Sleep(1); // check for new message every millisecond
	}

	return 0;
}

void CustomDialogBase::register_message_mapping(int message_t, std::function<void(void* pParam)> function_t)
{
	msg_to_function_map[message_t] = function_t;
}

