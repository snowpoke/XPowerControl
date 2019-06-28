#pragma once
// auto_rank_updater.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "stdafx.h"
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cmath>
#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <sstream>
#include <locale>

const std::string VERSION = "0.3";
enum mode { ZONES, TOWER, RAINMAKER, CLAMS };

//  libcurl write callback function
static int writer(char *data, size_t size, size_t nmemb, std::string *writerData);
std::string read_from_settings(std::string key_t);
std::string loadPage(std::string link_t, std::string SESSID_t);
void load_powers(float& zones_t, float& tower_t, float& rainmaker_t, float& clams_t, std::string SESSID_t);
void load_prev_match_info(int& start_time_t, float& power_after_t, std::string SESSID_t);

mode get_mode(std::string SESSID_t);
void save_info(std::string power_t, std::string win_t, std::string lose_t);
void load_ranges(float& zones_range_t, float& tower_range_t, float& rainmaker_range_t, float& clams_range_t);
void save_ranges(const float& zones_range_t, const float& tower_range_t, const float& rainmaker_range_t, const float& clams_range_t);
void save_info(float power_t);
void save_info(float power_t, float win_dist_t, float lose_dist_t);

float round2(float val_t);
UINT monitor_main(LPVOID pParam);
