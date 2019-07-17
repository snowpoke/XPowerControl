#include "stdafx.h"
#include <string>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"

using namespace std;

string read_from_settings(string key_t) {
	try {
		string ret;
		ifstream file("settings.txt");
		nlohmann::json j;
		file >> j;
		j.at(key_t).get_to(ret);
		return ret;
	}
	catch (...) {
		cout << "Failed to open settings file.";
		MessageBox(NULL, (LPCWSTR)L"Failed to open settings file.", (LPCWSTR)L"File error", MB_OK);

		return "";
	}
}