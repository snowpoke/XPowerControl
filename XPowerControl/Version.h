#pragma once
#include <string>
#include "stdafx.h"
#include <string>

class Version
{
public:
	int first, second, third, fourth;
	CString cstr_buffer;

	Version();

	static Version from_string(std::string ver_t); // generate from version string such as "1.0.0.0"
	static Version from_file(std::wstring file_t); // read from file using Windows API
	static Version from_int(int i1, int i2, int i3, int i4); // enter integer values for version directly
	void make_from_buffer();

	friend bool operator<(const Version& lhs, const Version& rhs);
	friend bool operator==(const Version& lhs, const Version& rhs);
	inline friend bool operator> (const Version& lhs, const Version& rhs) { return rhs < lhs; }
	inline friend bool operator<=(const Version& lhs, const Version& rhs) { return !(lhs > rhs); }
	inline friend bool operator>=(const Version& lhs, const Version& rhs) { return !(lhs < rhs); }
	inline friend bool operator!=(const Version& lhs, const Version& rhs) { return !(lhs == rhs); }

	std::wstring to_wstring(int digits);
	LPTSTR get_cstr_buffer(ULONG size);
};

