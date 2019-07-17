// auto_rank_updater.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
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
#include <assert.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cmath>
#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <sstream>
#include <locale>

using namespace std;
using json = nlohmann::json;


void load_powers(float& zones_t, float& tower_t, float& rainmaker_t, float& clams_t, string SESSID_t) {

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

		if (!j.at("splat_zones").at("my_ranking").is_null())
			j.at("splat_zones").at("my_ranking").at("x_power").get_to(zones_t);
		else zones_t = 0;

		if (!j.at("tower_control").at("my_ranking").is_null())
			j.at("tower_control").at("my_ranking").at("x_power").get_to(tower_t);
		else tower_t = 0;

		if (!j.at("rainmaker").at("my_ranking").is_null())
			j.at("rainmaker").at("my_ranking").at("x_power").get_to(rainmaker_t);
		else rainmaker_t = 0;

		if (!j.at("clam_blitz").at("my_ranking").is_null())
			j.at("clam_blitz").at("my_ranking").at("x_power").get_to(clams_t);
		else clams_t = 0;
	}
	catch (...) {
		AfxMessageBox(L"Failed to load the X rank powers.");
		AfxThrowUserException();
	}
}

void load_powers(ModeInfo<float>& powers_t, string SESSID_t) {
	load_powers(powers_t[Mode::ZONES], powers_t[Mode::TOWER], powers_t[Mode::RAINMAKER], powers_t[Mode::CLAMS], SESSID_t);
}

void load_prev_match_info(int& start_time_t, float& power_after_t, string SESSID_t) {
	string info_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/results", SESSID_t);
	try {
		json j = json::parse(info_json_string);
		string x_power_string;
		string type;
		j.at("results")[0].at("start_time").get_to(start_time_t);
		j.at("results")[0].at("type").get_to(type);
		if (type == "gachi" && !j.at("results")[0].at("x_power").is_null()) // check if the previous match was an x rank match
			j.at("results")[0].at("x_power").get_to(power_after_t);
		else
			power_after_t = 0;
	}
	catch (...) {
		AfxMessageBox(L"Failed to load the information about the previous match.");
		AfxThrowUserException();
	}

}


void load_prev_match_info(int& start_time_t, string SESSID_t) {
	float temp;
	load_prev_match_info(start_time_t, temp, SESSID_t);
}

Mode get_mode(string SESSID_t) {
	string schedule_json_string = http_requests::load_page("https://app.splatoon2.nintendo.net/api/schedules", SESSID_t);
	try {
		json j = json::parse(schedule_json_string);
		string rule;
		j.at("gachi")[0].at("rule").at("key").get_to(rule);
		ofstream file;
		file.open(read_from_settings("icon_file"));
		if (rule == "splat_zones") {
			file << "Mode_Icon_Splat_Zones.png";
			file.close();
			return ZONES;
		}
		else if (rule == "tower_control") {
			file << "Mode_Icon_Tower_Control.png";
			file.close();
			return TOWER;
		}
		else if (rule == "rainmaker") {
			file << "Mode_Icon_Rainmaker.png";
			file.close();
			return RAINMAKER;
		}
		else {
			file << "Mode_Icon_Clam_Blitz.png";
			file.close();
			return CLAMS;
		}
	}
	catch (...) {
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


void save_info(float power_t, optional<float> win_dist_t, optional<float> lose_dist_t, CXPowerControlDlg* window_t) {
	boost::format pwr_formatter("%.1f");
	pwr_formatter% power_t;
	string pwr_string = pwr_formatter.str();

	assert(win_dist_t.has_value() == lose_dist_t.has_value()); // the distances are either both given or not

	if (win_dist_t) {
		boost::format dist_formatter("%.0f");
		string win_dist_string, lose_dist_string;


		// if the win_dist is outside of <0 or >100, we display '??' instead
		if ((*win_dist_t < 0) || (*win_dist_t > 100)) {
			win_dist_string = "+??";
		}
		else if (*win_dist_t + *lose_dist_t > 37) { // if the range is above 35, we also display '??'
			win_dist_string = "+??";
		}
		else {
			dist_formatter% *win_dist_t;
			win_dist_string = "+" + dist_formatter.str();
		}
		dist_formatter% *lose_dist_t;
		lose_dist_string = "-" + dist_formatter.str();

		save_info(pwr_string, win_dist_string, lose_dist_string, window_t);
	}
	else {
		save_info(pwr_string, "", "", window_t);
	}
}


void save_info(string power_t, string win_t, string lose_t, CXPowerControlDlg* window_t) {
	ofstream file;
	file.open(read_from_settings("power_file"));
	file << power_t;
	file.close();

	file.open(read_from_settings("win_file"));
	file << win_t;
	file.close();

	file.open(read_from_settings("lose_file"));
	file << lose_t;
	file.close();

	file.open(read_from_settings("points_file"));
	file << power_t << "/n";
	file << win_t << "/n";
	file << lose_t;
	file.close();

	window_t->GetDlgItem(IDC_WINVALUE)->SetWindowTextW(s2ws(win_t).c_str());
	window_t->GetDlgItem(IDC_LOSEVALUE)->SetWindowTextW(s2ws(lose_t).c_str());
	window_t->GetDlgItem(IDC_POWERVALUE)->SetWindowTextW(s2ws(power_t).c_str());

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

	return abs(change_trunc);
}

UINT monitor_main_alt(LPVOID pParam) {

	CXPowerControlDlg* window = (CXPowerControlDlg*)pParam;

	window->m_loading_info.ShowWindow(SW_HIDE); // remove the loading indicator
	window->GetDlgItem(IDC_WINTEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_LOSETEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_BATTLE_START_TEXT)->ShowWindow(SW_SHOW);
	
	string SESSID = read_from_settings("iksm-session");

	ModeInfo<float> powers, ranges;
	load_powers(powers, SESSID);
	load_ranges(ranges);
	Mode curr_mode = get_mode(SESSID);

	save_info(powers[curr_mode], {}, {}, window);

	// we set up the start time of the previous match so we can recognize when a match has ended
	int prev_start_time;
	load_prev_match_info(prev_start_time, SESSID);

	// we initialize powers_after for use later, it contains the powers we would have after a loss
	ModeInfo<float> powers_after_loss = {0,0,0,0};

	while (!window->kill_monitor_main) {
		if (!window->match_running) {

			// if a new rotation was detected, we reload the mode and save the power
			if (window->update_rotation) {
				curr_mode = get_mode(SESSID);
				save_info(powers[curr_mode], {}, {}, window);
				window->update_rotation = false;
			}

			load_powers(powers_after_loss, SESSID);

			// a change in the saved power has been detected
			if (powers[curr_mode] != powers_after_loss[curr_mode]) {
				window->match_running = true;

				// number of points that would be lost after a defeat (contains negative sign)
				float loss_distance = round2(powers[curr_mode] - powers_after_loss[curr_mode]);
				// estimate of number of points that would be won after a victory
				float win_distance = ranges[curr_mode] - loss_distance;

				// truncate numbers into the format used in Splatoon
				float loss_trunc = power_truncate(-1*loss_distance, powers[curr_mode]);
				float win_trunc = power_truncate(win_distance, powers[curr_mode]);
				cout << "Current power:" << powers[curr_mode] << "/n";
				cout << "Loss distance (float): " << loss_distance << "/n";
				cout << "Loss distance (trunc): " << loss_trunc << "/n";
				cout << "Win distance (float): " << win_distance << "/n";
				cout << "Win distance (trunc): " << win_trunc << "/n/n/n";
				save_info(powers[curr_mode], win_trunc, loss_trunc, window);
			}
		}
		else { // no match is currently running

			// load starting time and resulting power of the last match saved in splatapp
			int new_start_time;
			float new_power;
			load_prev_match_info(new_start_time, new_power, SESSID);

			if (prev_start_time != new_start_time) { // if a new starting time was found, this means the match has ended
				window->match_running = false;

				float power_change = new_power - powers[curr_mode];
				powers[curr_mode] = new_power;

				// initialize analytics thread
				//optional<float> win_change = (power_change > 0) ? optional<float>(power_change) : optional<float>();
				//args_analytics args = { &(window->analytics), SESSID, powers_after_loss[curr_mode], ranges[curr_mode], win_change };
				//AfxBeginThread(add_recent_match_analytics, &args);


				if (power_change > 0) { // if we won the match, we can use this information to adjust the range
					ranges[curr_mode] = round2(new_power - powers_after_loss[curr_mode]);
					save_ranges(ranges);
				}

				prev_start_time = new_start_time;
				save_info(powers[curr_mode], {}, {}, window);
			}
		}

		// this shouldn't be necessary, but somehow is
		window->Invalidate();
		window->UpdateWindow();

		Sleep(1000);
	}

	return 0;
}
/*
UINT monitor_main(LPVOID pParam)
{
	cout << "-- PRE-RELEASE VERSION FOR SUBSCRIBERS ONLY --\n"
		<< "VERSION X.X \n"
		<< "This program is a very early version and is only available to subscribers of my twitch channel:\n"
		<< "https://twitch.tv/snowpoke\n"
		<< "If you obtained this program thorugh other means, please consider leaving a gift sub to my wonderful community.\n\n";

	//cout << xpower_json_string.c_str();
	float zones_pwr, tower_pwr, rainmaker_pwr, clams_pwr;
	float zones_pwr_after, tower_pwr_after, rainmaker_pwr_after, clams_pwr_after;
	int start_time;
	float power_after_loss;
	float power_after;
	float power_before;
	float loss_distance = 0;
	float win_distance = 0;

	CXPowerControlDlg* window = (CXPowerControlDlg*)pParam;

	string SESSID = read_from_settings("iksm-session");


	load_powers(zones_pwr, tower_pwr, rainmaker_pwr, clams_pwr, SESSID);

	load_prev_match_info(start_time, power_after, SESSID);


	Mode curr_mode = get_mode(SESSID);
	float zones_range, tower_range, rainmaker_range, clams_range;
	float* curr_range;

	load_ranges(zones_range, tower_range, rainmaker_range, clams_range);

	curr_range = &zones_range; // just so not all initializations of curr_range are in if clauses
	if (curr_mode == ZONES) {
		save_info(zones_pwr, window);
		curr_range = &zones_range;
	}
	else if (curr_mode == TOWER) {
		save_info(tower_pwr, window);
		curr_range = &tower_range;
	}
	else if (curr_mode == RAINMAKER) {
		save_info(rainmaker_pwr, window);
		curr_range = &rainmaker_range;
	}
	else if (curr_mode == CLAMS) {
		save_info(clams_pwr, window);
		curr_range = &clams_range;
	}

	window->m_loading_info.ShowWindow(SW_HIDE); // remove the loading indicator
	window->GetDlgItem(IDC_WINTEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_LOSETEXT)->ShowWindow(SW_SHOW);
	window->GetDlgItem(IDC_BATTLE_START_TEXT)->ShowWindow(SW_SHOW);
	cout << "Monitoring for start of a match...";

	while (!window->kill_monitor_main) {
		if (!window->match_running) {
			if (window->update_rotation) {
				load_ranges(zones_range, tower_range, rainmaker_range, clams_range);

				curr_mode = get_mode(SESSID);
				curr_range = &zones_range; // just so not all initializations of curr_range are in if clauses
				if (curr_mode == ZONES) {
					save_info(zones_pwr, window);
					curr_range = &zones_range;
				}
				else if (curr_mode == TOWER) {
					save_info(tower_pwr, window);
					curr_range = &tower_range;
				}
				else if (curr_mode == RAINMAKER) {
					save_info(rainmaker_pwr, window);
					curr_range = &rainmaker_range;
				}
				else if (curr_mode == CLAMS) {
					save_info(clams_pwr, window);
					curr_range = &clams_range;
				}
				window->update_rotation = false;
				continue;
			}
			// if there is no match running, check for changes in the x power
			// once a power changes, we know that a match is running
			load_powers(zones_pwr_after, tower_pwr_after, rainmaker_pwr_after, clams_pwr_after, SESSID);
			float pwr_sum_old = round2(zones_pwr + tower_pwr + rainmaker_pwr + clams_pwr);
			float pwr_sum = round2(zones_pwr_after + tower_pwr_after + rainmaker_pwr_after + clams_pwr_after);
			if (pwr_sum_old != pwr_sum) {
				cout << "A new match has started." << "\n";
				window->match_running = true;
				if (zones_pwr != zones_pwr_after) {
					loss_distance = round2(zones_pwr - zones_pwr_after);
					power_before = zones_pwr;
					power_after_loss = zones_pwr_after;
				}
				else if (tower_pwr != tower_pwr_after) {
					loss_distance = round2(tower_pwr - tower_pwr_after);
					power_before = tower_pwr;
					power_after_loss = tower_pwr_after;
				}
				else if (rainmaker_pwr != rainmaker_pwr_after) {
					loss_distance = round2(rainmaker_pwr - rainmaker_pwr_after);
					power_before = rainmaker_pwr;
					power_after_loss = rainmaker_pwr_after;
				}
				else if (clams_pwr != clams_pwr_after) {
					loss_distance = round2(clams_pwr - clams_pwr_after);
					power_before = clams_pwr;
					power_after_loss = clams_pwr_after;
				}
				win_distance = round2(*curr_range - loss_distance);
				cout << "Current power: " << power_before << "\n";
				cout << "Loss distance: -" << loss_distance << "\n";
				cout << "Win distance: " << win_distance << "\n";

				float power_trunc, loss_trunc, win_trunc;
				power_trunc = trunc(power_before);
				loss_trunc = power_trunc - trunc(power_before - loss_distance);
				win_trunc = trunc(power_before + win_distance) - power_trunc;
				save_info(power_before, win_trunc, loss_trunc, window);
				cout << "Monitoring for end of the match...";
			}

		}
		else {
			// if there is a match running, we test for a new entry in the match list - this indicates that the match is over
			int start_time_old = start_time;

			load_prev_match_info(start_time, power_after, SESSID);
			if (start_time != start_time_old) { // a new match has been stored
				cout << "Match has ended." << "\n";
				window->match_running = false;
				// if we won the match, we can use this to adjust the range
				float range_old = *curr_range;
				float power_change = power_after - power_before;

				args_analytics args = { window->analytics, SESSID, win_distance, range_old, power_change };
				AfxBeginThread(add_recent_match_analytics, &args);

				if (power_change > 0) {
					*curr_range = round2(power_after - power_after_loss);
					cout << "New range: " << *curr_range << "\n";
					if (range_old != *curr_range) {
						cout << "Adjusted range by " << *curr_range - range_old << "\n";
						save_ranges(zones_range, tower_range, rainmaker_range, clams_range);

					}
				}

				cout << "\n----------\n";
				load_powers(zones_pwr, tower_pwr, rainmaker_pwr, clams_pwr, SESSID); // update all powers
				loss_distance = 0;
				win_distance = 0;
				save_info(power_after, window);

				cout << "Monitoring for start of a match...";
			}

		}
		Sleep(1000);
	}
	return 0;
}
*/