#pragma once
// Regularly checks information about the matches and predicts loss and win for the next match
#include "stdafx.h"
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "XPowerControlDlg.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cmath>
#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <sstream>
#include <locale>

enum Mode { ZONES, TOWER, RAINMAKER, CLAMS };


template<class T>
using ModeInfo = std::array<T, 4>;

// loads battle information
void load_powers(float& zones_t, float& tower_t, float& rainmaker_t, float& clams_t, const ModeInfo<float>& ranges_t, std::string SESSID_t);
void load_powers(ModeInfo<float>& powers_t, const ModeInfo<float>& ranges_t, std::string SESSID_t);
void load_prev_match_info(int& start_time_t, float& power_after_t, std::string SESSID_t);
void load_prev_match_info(int& start_time_t, std::string SESSID_t);
Mode get_mode(std::string SESSID_t);
void load_ranges(float& zones_range_t, float& tower_range_t, float& rainmaker_range_t, float& clams_range_t);
void load_ranges(ModeInfo<float>& ranges);

// saves ranges into text files and sends them to the dialog
void save_ranges(const float& zones_range_t, const float& tower_range_t, const float& rainmaker_range_t, const float& clams_range_t);
void save_ranges(const ModeInfo<float>& ranges);
void save_info(float power_t, std::optional<float> win_trunc_t, std::optional<float> lose_trunc_t, std::optional<float> win_dist_t, std::optional<float> lose_dist_t, CXPowerControlDlg* window_t);
void save_info(std::string power_t, std::string win_t, std::string lose_t, std::string win_detail_t, std::string lose_detail_t, CXPowerControlDlg* window_t);

// rounds the power after the second decimal to avoid float precision errors
float round2(float val_t);

// turns power difference into differnce of numbers before the decimal point
float power_truncate(float change_t, float power_t);

// monitor function
UINT monitor_main_alt(LPVOID pParam);
UINT monitor_main(LPVOID pParam);
