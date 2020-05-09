// auto_rank_updater.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "stdafx.h"
#include "monitor.h"
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "XPowerControl.h"
#include "XPowerControlDlg.h"
#include "RetrieveTokenDlg.h"
#include "stdafx.h"
#include "Analytics.h"
#include "wstring_transform.h"
#include "settings.h"
#include "http_requests.h"
#include "logging.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cmath>
#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <sstream>
#include <locale>
#include <codecvt>

using namespace std;
using json = nlohmann::json;


void load_powers(float& zones_t, float& tower_t, float& rainmaker_t, float& clams_t, const ModeInfo<float>& ranges_t, string SESSID_t) {
	auto _logger = logging::get_logger(DEFAULT_LOG);

	boost::gregorian::date utc_today(boost::gregorian::day_clock::universal_day());
	string start_year = to_string(utc_today.year() - 2000);
	string start_month = to_string(utc_today.month());
	boost::gregorian::date_duration dd(1); // duration of 1 day
	boost::gregorian::date utc_nextmonth = utc_today.end_of_month() + dd;
	string end_year = to_string(utc_nextmonth.year() - 2000);
	string end_month = to_string(utc_nextmonth.month());

	// if the month is below 10, add a '0' to the start of the string
	if (utc_today.month() < 10)
		start_month = '0' + start_month;
	if (utc_nextmonth.month() < 10)
		end_month = '0' + end_month;

	string rotation_string = start_year + start_month + "01T00_" + end_year + end_month + "01T00";

	string xpower_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/x_power_ranking/" + rotation_string + "/summary", SESSID_t);
	try {
		json j = json::parse(xpower_json_string);
		ModeInfo<float>& new_ranges = ModeInfo<float>(ranges_t);

		if (!j.at("splat_zones").at("my_ranking").is_null())
			j.at("splat_zones").at("my_ranking").at("x_power").get_to(zones_t);
		else {
			zones_t = 0;
			new_ranges[Mode::ZONES] = 0;
		}

		if (!j.at("tower_control").at("my_ranking").is_null())
			j.at("tower_control").at("my_ranking").at("x_power").get_to(tower_t);
		else {
			tower_t = 0;
			new_ranges[Mode::TOWER] = 0;
		}

		if (!j.at("rainmaker").at("my_ranking").is_null())
			j.at("rainmaker").at("my_ranking").at("x_power").get_to(rainmaker_t);
		else {
			rainmaker_t = 0;
			new_ranges[Mode::RAINMAKER] = 0;
		}

		if (!j.at("clam_blitz").at("my_ranking").is_null())
			j.at("clam_blitz").at("my_ranking").at("x_power").get_to(clams_t);
		else {
			clams_t = 0;
			new_ranges[Mode::CLAMS] = 0;
		}

		// reset ranges for modes where the power has been reset
		if (ranges_t[Mode::ZONES] != new_ranges[Mode::ZONES]
			|| ranges_t[Mode::TOWER] != new_ranges[Mode::TOWER]
			|| ranges_t[Mode::RAINMAKER] != new_ranges[Mode::RAINMAKER]
			|| ranges_t[Mode::CLAMS] != new_ranges[Mode::CLAMS]) {
			save_ranges(new_ranges);
		}
	}
	catch (exception& e) {
		_logger->error("Failed to load the current X rank powers. e.what() responded: {}\nJSON result: {}", e.what(), xpower_json_string);
		AfxMessageBox(L"Failed to load the current X rank powers.");
		AfxThrowUserException();
	}
}

void load_powers(ModeInfo<float>& powers_t, const ModeInfo<float>& ranges_t, string SESSID_t) {
	load_powers(powers_t[Mode::ZONES], powers_t[Mode::TOWER], powers_t[Mode::RAINMAKER], powers_t[Mode::CLAMS], ranges_t, SESSID_t);
}

void load_prev_match_info(int& start_time_t, optional<float>& power_after_t, string SESSID_t) {
	auto _logger = logging::get_logger(DEFAULT_LOG);
	auto _jsonlogger = logging::get_logger(JSON_LOG);

	string info_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/results", SESSID_t, false);
	try {
		json j = json::parse(info_json_string);
		// we write data included in j.at("results")[0] into the JSON log
		_jsonlogger->info("j.at(\"results\")[0] contains: {}", j.at("results")[0].dump());
		start_time_t = j.at("results")[0].at("start_time").get<int>();
		string type = j.at("results")[0].at("type").get<string>();
		if (type == "gachi" && !j.at("results")[0].at("x_power").is_null()) // check if the previous match was an x rank match
			power_after_t = j.at("results")[0].at("x_power").get<float>();
		else
			power_after_t = {};
	}
	catch (exception& e) {
		_logger->error("Failed to load the information about the previous match. e.what() responded: {}\nJSON result: {}",
			e.what(), info_json_string);
		AfxMessageBox(L"Failed to load the information about the previous match.");
		AfxThrowUserException();
	}

}


void load_prev_match_info(int& start_time_t, string SESSID_t) {
	optional<float> temp;
	load_prev_match_info(start_time_t, temp, SESSID_t);
}

Mode get_mode(string SESSID_t) {
	auto _logger = logging::get_logger(DEFAULT_LOG);
	string schedule_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/schedules", SESSID_t);
	try {
		json j = json::parse(schedule_json_string);
		string rule;
		j.at("gachi")[0].at("rule").at("key").get_to(rule);
		ofstream file;

		// save mode icon location
		if (read_from_settings<bool>("do_savematchdata")) {
			file.open(read_from_settings<string>("matchdata_directory") + "icon.txt");
			if (rule == "splat_zones") {
				file << "Mode_Icon_Splat_Zones.png";
				file.close();
			}
			else if (rule == "tower_control") {
				file << "Mode_Icon_Tower_Control.png";
				file.close();
			}
			else if (rule == "rainmaker") {
				file << "Mode_Icon_Rainmaker.png";
				file.close();
			}
			else {
				file << "Mode_Icon_Clam_Blitz.png";
				file.close();
			}
		}

		// return mode
		_logger->info("Looked up mode and retrieved JSON result \"{}\"", rule);

		if (rule == "splat_zones") {
			return ZONES;
		}
		else if (rule == "tower_control") {
			return TOWER;
		}
		else if (rule == "rainmaker") {
			return RAINMAKER;
		}
		else {
			return CLAMS;
		}

	}
	catch (exception& e) {
		_logger->error("Failed to load the current mode. e.what() responded: {}\nJSON content: {}", e.what(), schedule_json_string);
		AfxMessageBox(L"Failed to load the current mode.");
		AfxThrowUserException();
		return ZONES;
	}
}

void load_ranges(float& zones_range_t, float& tower_range_t, float& rainmaker_range_t, float& clams_range_t) {
	ifstream file;
	file.open("ranges.txt", ifstream::in);
	file >> zones_range_t;
	file >> tower_range_t;
	file >> rainmaker_range_t;
	file >> clams_range_t;
	file.close();
}

void load_ranges(ModeInfo<float>& ranges_t) {
	load_ranges(ranges_t[Mode::ZONES], ranges_t[Mode::TOWER], ranges_t[Mode::RAINMAKER], ranges_t[Mode::CLAMS]);
}

std::string mode_to_string(Mode m)
{
	switch (m) {
	case ZONES:
		return "Splat Zones";
		break;
	case TOWER:
		return "Tower Control";
		break;
	case RAINMAKER:
		return "Rainmaker";
		break;
	case CLAMS:
		return "Clam Blitz";
		break;
	default:
		return "Mode not registered";
		break;
	}
}

void save_ranges(const float& zones_range_t, const float& tower_range_t, const float& rainmaker_range_t, const float& clams_range_t) {
	ofstream file;
	file.open("ranges.txt");
	file << zones_range_t << "\n";
	file << tower_range_t << "\n";
	file << rainmaker_range_t << "\n";
	file << clams_range_t << "\n";
	file.close();
}

void save_ranges(const ModeInfo<float>& ranges_t) {
	save_ranges(ranges_t[Mode::ZONES], ranges_t[Mode::TOWER], ranges_t[Mode::RAINMAKER], ranges_t[Mode::CLAMS]);
}


void save_info(float power_t, optional<float> win_trunc_t, optional<float> lose_trunc_t, optional<float> win_dist_t, optional<float> lose_dist_t, CXPowerControlDlg* window_t) {
	boost::format pwr_formatter("%.1f");
	pwr_formatter% power_t;
	string pwr_string = pwr_formatter.str();

	assert(win_trunc_t.has_value() == lose_trunc_t.has_value()); // the distances are either both given or not

	if (win_trunc_t) {
		boost::format dist_formatter("%.0f");
		boost::format dist_formatter_detailed("%.1f");
		string win_dist_string, lose_dist_string;
		string win_detail_string, lose_detail_string;


		// if the win_dist is outside of <0 or >100, we display '??' instead
		// same if the spread is larger than 40, or whatever the user has specified
		bool limit_spread = read_from_settings<bool>("do_hidepower");
		int max_spread = read_from_settings<int>("max_precise");

		if ((*win_dist_t + *lose_dist_t < 10) || (*win_dist_t > 100) || (limit_spread && *win_dist_t + *lose_dist_t > max_spread)) {
			dist_formatter_detailed%* win_dist_t;
			win_dist_string = "+??";
			string win_detailed = dist_formatter_detailed.str();
			if (*win_dist_t >= 0)
				win_detail_string = "Calibrating [+" + win_detailed + "]";
			else
				win_detail_string = "Calibrating";
		}
		else {
			win_trunc_t = max(0, win_trunc_t); // makes sure that win_trunc_t is not a negative number
			dist_formatter%* win_trunc_t;
			win_dist_string = "+" + dist_formatter.str();
			win_detail_string = "";
		}
		dist_formatter% abs(*lose_trunc_t);
		dist_formatter_detailed% *lose_dist_t;
		lose_dist_string = "-" + dist_formatter.str();
		lose_detail_string = "-" + dist_formatter_detailed.str();

		save_info(pwr_string, win_dist_string, lose_dist_string, win_detail_string, lose_detail_string, window_t);
	}
	else {
		save_info(pwr_string, "", "", "", "", window_t);
	}
}


void save_info(string power_t, string win_t, string lose_t, string win_detail_t, string lose_detail_t, CXPowerControlDlg* window_t) {
	ofstream file;
	string data_dir = read_from_settings<string>("matchdata_directory");


	if (read_from_settings<bool>("do_savematchdata")) {
		file.open(data_dir + "power.txt");
		file << power_t;
		file.close();

		file.open(data_dir + "win.txt");
		file << win_t;
		file.close();

		file.open(data_dir + "lose.txt");
		file << lose_t;
		file.close();
	}

	window_t->GetDlgItem(IDC_WINVALUE)->SetWindowTextW(transform::s2ws(win_t).c_str());
	window_t->GetDlgItem(IDC_LOSEVALUE)->SetWindowTextW(transform::s2ws(lose_t).c_str());
	window_t->GetDlgItem(IDC_POWERVALUE)->SetWindowTextW(transform::s2ws(power_t).c_str());
	window_t->GetDlgItem(IDC_WINDETAILS)->SetWindowTextW(transform::s2ws(win_detail_t).c_str());
	window_t->GetDlgItem(IDC_LOSEDETAILS)->SetWindowTextW(transform::s2ws(lose_detail_t).c_str());
	

	if (window_t->match_running)
		window_t->GetDlgItem(IDC_BATTLE_START_TEXT)->ShowWindow(SW_HIDE);
	else
		window_t->GetDlgItem(IDC_BATTLE_START_TEXT)->ShowWindow(SW_SHOW);

	window_t->Invalidate();
	window_t->UpdateWindow();
}

float round2(float val_t) {
	return static_cast<float>(round(val_t * 10.0) / 10.0);
}

float power_truncate(float change_t, float power_t) {
	float power_trunc = trunc(power_t);
	float change_trunc = trunc(power_t + change_t) - power_trunc;

	return change_trunc;
}


void run_script_matchstart() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (read_from_settings<bool>("do_startcmd")) {
		string start_script = read_from_settings<string>("start_cmd");
		if (start_script != "") {
			CreateProcess(NULL,
				const_cast<LPWSTR>(transform::s2ws(start_script).c_str()),
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
}

void run_script_matchend() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (read_from_settings<bool>("do_endcmd")) {
		string end_script = read_from_settings<string>("end_cmd");
		if (end_script != "") {
			CreateProcess(NULL,
				const_cast<LPWSTR>(transform::s2ws(end_script).c_str()),
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
}

UINT monitor_main_alt(LPVOID pParam) {
	logging::log_ptr _logger = logging::get_logger(DEFAULT_LOG);


	CXPowerControlDlg* window = (CXPowerControlDlg*)pParam;

	window->m_loading_info.ShowWindow(SW_HIDE); // remove the loading indicator
	window->GetDlgItem(IDC_WINTEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_LOSETEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_BATTLE_START_TEXT)->ShowWindow(SW_SHOW);
	
	string SESSID = read_from_settings<string>("iksm-session");

	ModeInfo<float> powers, ranges;
	load_ranges(ranges);
	load_powers(powers, ranges, SESSID);
	Mode curr_mode = get_mode(SESSID);

	save_info(powers[curr_mode], {}, {}, {}, {}, window);

	// we set up the start time of the previous match so we can recognize when a match has ended
	int prev_start_time;
	load_prev_match_info(prev_start_time, SESSID);

	// we initialize powers_after for use later, it contains the powers we would have after a loss
	ModeInfo<float> powers_after_loss = {0,0,0,0};

	int invalidate_timer = 0;
	while (!window->kill_monitor_main) {
		if (!window->match_running) { // no match is currently running

			// if a new rotation was detected, we reload the mode and save the power
			if (window->update_rotation) {
				curr_mode = get_mode(SESSID);
				save_info(powers[curr_mode], {}, {}, {}, {}, window);
				window->update_rotation = false;
			}

			load_powers(powers_after_loss, ranges, SESSID);

			// a change in the saved power has been detected
			if (powers[curr_mode] != powers_after_loss[curr_mode]) {
				window->match_running = true;
				run_script_matchstart();
				_logger->info("Start of match has been detected.");
				// number of points that would be lost after a defeat (contains negative sign)
				float loss_distance = round2(powers[curr_mode] - powers_after_loss[curr_mode]);
				// estimate of number of points that would be won after a victory
				float win_distance = ranges[curr_mode] - loss_distance;

				// truncate numbers into the format used in Splatoon
				float loss_trunc = power_truncate(-1*loss_distance, powers[curr_mode]);
				float win_trunc = power_truncate(win_distance, powers[curr_mode]);
				_logger->info("Current power({}): {}", mode_to_string(curr_mode), powers[curr_mode]);
				_logger->info("Loss distance (float): {}", loss_distance);
				_logger->info("Loss distance (trunc): {}", loss_trunc);
				_logger->info("Win distance (float): {}", win_distance);
				_logger->info("Win distance (trunc): {}", win_trunc);
				save_info(powers[curr_mode], win_trunc, loss_trunc, win_distance, loss_distance, window);
			}
		}
		else { 

			// load starting time and resulting power of the last match saved in splatapp
			int new_start_time;
			optional<float> new_power;
			load_prev_match_info(new_start_time, new_power, SESSID);

			if (prev_start_time != new_start_time && new_power) { // if a new starting time was found, this means the match has ended
				window->match_running = false;
				run_script_matchend();
				_logger->info("End of match has been detected.");
				_logger->info("Detected new power: {}", *new_power);
				_logger->info("Previous power({}): {}", mode_to_string(curr_mode), powers[curr_mode]);
				float power_change = *new_power - powers[curr_mode];
				powers[curr_mode] = *new_power;

				// initialize analytics thread
				//optional<float> win_change = (power_change > 0) ? optional<float>(power_change) : optional<float>();
				//args_analytics args = { &(window->analytics), SESSID, powers_after_loss[curr_mode], ranges[curr_mode], win_change };
				//AfxBeginThread(add_recent_match_analytics, &args);


				if (power_change > 0) { // if we won the match, we can use this information to adjust the range
					auto old_range = ranges[curr_mode];
					ranges[curr_mode] = round2(*new_power - powers_after_loss[curr_mode]);
					_logger->info("Adjusted range in {} from {} to {}.", mode_to_string(curr_mode), old_range, ranges[curr_mode]);
					save_ranges(ranges);
				}

				prev_start_time = new_start_time;
				save_info(powers[curr_mode], {}, {}, {}, {}, window);
			}
		}

		// refreshes the UI: this shouldn't be necessary, but somehow is
		if (invalidate_timer > 3000) {
			window->Invalidate();
			window->UpdateWindow();
			invalidate_timer = 0;
		}
		invalidate_timer += 1000;

		Sleep(1000);
	}

	return 0;
}