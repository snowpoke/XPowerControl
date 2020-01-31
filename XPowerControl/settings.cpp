#include "stdafx.h"
#include <string>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include <optional>

using namespace std;

bool settings_exist(string key_t) {
	try {
		bool ret;
		ifstream file("settings.txt");
		nlohmann::json j;
		file >> j;
		ret = (j.find(key_t) != j.end());
		return ret;
	}
	catch (...) {
		cout << "Failed to open settings file.";
		MessageBox(NULL, (LPCWSTR)L"Failed to open settings file.", (LPCWSTR)L"File error", MB_OK);

		return false;
	}
}