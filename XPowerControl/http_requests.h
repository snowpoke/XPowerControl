#pragma once
#include <string>
#include <curl/curl.h>
#include "CustomDialogBase.h"
#include "logging.h"

namespace http_requests {
	//  libcurl write callback function
	static int writer(char* data, size_t size, size_t nmemb, std::string* writerData);

	// sends http get request with iksm_token cookie
	std::string load_page(const std::string& link_t, const std::string& SESSID_t);

	// read json parameter from version.json file on github
	std::string get_global_info(const std::string& key_t);

	// Callback for downloads
	class Callback : public IBindStatusCallback {
	private:
		logging::log_ptr _logger;
	public:
		unsigned long progress = 0;
		unsigned long max_dl = 0;
		unsigned long status_code = 0;
		bool finished = false;
		bool cancel_download = false;
		CustomDialogBase* dlg;


		Callback();
		Callback(CustomDialogBase* dlg_t);

		void cancel(); // cancel download

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

	struct DownloadParam {
		std::wstring dl_URL;
		std::wstring savepath_t;
		Callback* callback_t;
	};

	UINT thread_download_file(LPVOID pParam);
	
}