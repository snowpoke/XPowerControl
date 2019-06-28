#include "stdafx.h"
#include "monitor.h"
#include "rotation_monitor.h"
#include "XPowerControlDlg.h"
#include "nlohmann/json.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <ctime>
#include <fstream>

using namespace std;
using namespace boost;

void write_time(string time) {
	ofstream file;
	file.open("C:/Users/fkpma/Documents/Twitch Overlay/countdown.txt");
	file << time;
	file.close();
}

void write_timestamp_end(int timestamp) {
	ofstream file;
	file.open("C:/Users/fkpma/Documents/Twitch Overlay/timestamp_end.txt");
	file << timestamp;
	file.close();
}

UINT monitor_rotation(LPVOID pParam) {

	CXPowerControlDlg* window = (CXPowerControlDlg*)pParam;
	string SESSID = read_from_settings("iksm-session");

	// check current rotation
	string schedule_string = loadPage("https://app.splatoon2.nintendo.net/api/schedules",SESSID);
	int rot_end;
	nlohmann::json j;
	try {
		j = nlohmann::json::parse(schedule_string);
		rot_end = j["gachi"].at(0)["end_time"].get<int>();
	}
	catch (...) {
		AfxMessageBox(L"Failed to load current rotation.");
		AfxThrowUserException();
	}
	time_t timestamp_now = time(nullptr);
	int rot_seconds_left = static_cast<int>(rot_end - timestamp_now);

	write_timestamp_end(rot_end);

	while (!window->kill_rotation_monitor) {
		if (rot_seconds_left <= 0) {
			if (window->match_running) { // the rotation is over, but we are still in a match => display "FINAL"
				write_time("FINAL");
			}
			else { // if no match is running, we update to the next rotation
				schedule_string = loadPage("https://app.splatoon2.nintendo.net/api/schedules", SESSID);
				try {
					j = nlohmann::json::parse(schedule_string);
					rot_end = j["gachi"].at(0)["end_time"].get<int>();
				}
				catch (...) {
					AfxMessageBox(L"Failed to load current rotation.");
					AfxThrowUserException();
				}
				timestamp_now = time(nullptr);
				rot_seconds_left = static_cast<int>(rot_end - timestamp_now);
				window->update_rotation = true;
				write_timestamp_end(rot_end);
			}
		}
		else {
			timestamp_now = time(nullptr);
			rot_seconds_left = static_cast<int>(rot_end - timestamp_now);
			posix_time::time_duration td = posix_time::seconds(rot_seconds_left);
			string countdown = posix_time::to_simple_string(td);
			if (td.hours() == 0)
				countdown = countdown.substr(3, 7);
			else
				countdown = countdown.substr(1, 7);
			write_time(countdown);
		}
		Sleep(1000);
	}
	return 0;
}
