#include "Version.h"
#include "Windows.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include "wstring_transform.h"
#include <vector>
#include <sstream>
#include <iostream>
#include <string>

using namespace std;


// code from https://thispointer.com/how-to-split-a-string-in-c/
vector<string> version_split(string strToSplit, char delimeter)
{
	stringstream ss(strToSplit);
	string item;
	vector<string> splittedStrings;
	while (getline(ss, item, delimeter))
	{
		splittedStrings.push_back(item);
	}
	return splittedStrings;
}


wstring Version::to_wstring(int digits)
{
	wstring out_string;
	switch (digits) {
	case 4:
		out_string = L"." + ::to_wstring(fourth);
	case 3:
		out_string = L"." + ::to_wstring(third) + out_string;
	case 2:
		out_string = L"." + ::to_wstring(second) + out_string;
	case 1:
		out_string = ::to_wstring(first) + out_string;
		break;
	default:
		assert(false);
		out_string = L"err";
	}

	return out_string;
}

LPTSTR Version::get_cstr_buffer(ULONG size)
{
	return cstr_buffer.GetBuffer(size);
}

Version::Version() {
	first = -1;
	second = -1;
	third = -1;
	fourth = -1;
}

Version Version::from_string(std::string ver_t) {
	vector<string> version_nums = version_split(ver_t, '.');
	// we add the values 0, 0, 0, 0 to the vector so that it always has at least 4 elements
	version_nums.push_back("0");
	version_nums.push_back("0");
	version_nums.push_back("0");
	version_nums.push_back("0");

	int i1 = stoi(version_nums[0]);
	int i2 = stoi(version_nums[1]);
	int i3 = stoi(version_nums[2]);
	int i4 = stoi(version_nums[3]);

	return Version::from_int(i1, i2, i3, i4);
}


Version Version::from_file(std::wstring file_t)
{
	struct LANGARRAY
	{
		WORD language_id;
		WORD character_set;
	};

	DWORD temp_handle = 1;
	DWORD version_size = GetFileVersionInfoSize(file_t.c_str(), &temp_handle);
	auto version_info_buffer = make_unique<BYTE[]>(version_size);
	GetFileVersionInfo(file_t.c_str(), NULL, version_size, version_info_buffer.get());

	// get language
	VOID* lp_lang_info;
	UINT cb_lang;
	WCHAR req_string[WCHAR_MAX];
	VerQueryValue(version_info_buffer.get(), L"\\VarFileInfo\\Translation", &lp_lang_info, &cb_lang);

	// get location for ProductVersion
	LANGARRAY* lang_info = (LANGARRAY*)lp_lang_info;
	wsprintf(req_string, L"\\StringFileInfo\\%04x%04x\\ProductVersion", lang_info->language_id, lang_info->character_set);

	// obtain version string
	LPTSTR version;
	UINT cb_version;
	VerQueryValue(version_info_buffer.get(), req_string, (LPVOID*) &version, &cb_version);

	// Split version into numbers by . character
	return Version::from_string(transform::ws2s(version));
}

Version Version::from_int(int i1, int i2, int i3, int i4)
{
	Version v = Version();
	v.first = i1;
	v.second = i2;
	v.third = i3;
	v.fourth = i4;
	return v;
}

void Version::make_from_buffer()
{
	string version_string = transform::ws2s(wstring(cstr_buffer));
	Version v = from_string(version_string);
	first = v.first;
	second = v.second;
	third = v.third;
	fourth = v.fourth;
}

bool operator<(const Version& lhs, const Version& rhs)
{
	array vec_lhs = { lhs.first, lhs.second, lhs.third, lhs.fourth };
	array vec_rhs = { rhs.first, rhs.second, rhs.third, rhs.fourth };
	return lexicographical_compare(vec_lhs.begin(), vec_lhs.end(),
		vec_rhs.begin(), vec_rhs.end());
}

bool operator==(const Version& lhs, const Version& rhs)
{
	return (lhs.first == rhs.first
		&& lhs.second == rhs.second
		&& lhs.third == rhs.third
		&& lhs.fourth == rhs.fourth);
}
