#pragma once
#include "stdafx.h"
#include <string>


void write_time(const std::string& time, const std::string& filename);
void write_timestamp_end(int timestamp, const std::string& filename);
UINT monitor_rotation(LPVOID pParam);