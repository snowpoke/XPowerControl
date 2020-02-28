#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include "wstring_transform.h"
#include <optional>
#define CURRENT_SETTING_VERSION 4

bool settings_exist(std::string key_t); // checks if a certain key exists in the settings file

template <class T>
T read_from_settings(std::string key_t) {
	try {
		T ret;
		std::ifstream file("settings.txt");
		nlohmann::json j;
		file >> j;
		j.at(key_t).get_to(ret);
		return ret;
	}
	catch (...) {
		std::cout << "Failed to read item '" << key_t << "' from settings file.";
		MessageBox(NULL, (LPCWSTR)((L"Failed to read item '" + transform::s2ws(key_t) + L"' from settings file.").c_str()),
			(LPCWSTR)L"File error", MB_OK);

		return T();
	}
}

template <class T>
T read_from_settings(std::string key_t, T alternative_t) {
	if (settings_exist(key_t))
		return read_from_settings<T>(key_t);
	else
		return alternative_t;
}