// Saves analytics/telemetry about the user and saves it to a data file.
// In a later version, this is also supposed to upload it to my server.

#include "Analytics.h"
#include "nlohmann/json.hpp"
#include "monitor.h"
#include "XPowerControlDlg.h"
#include "http_requests.h"
#include "boost/archive/binary_oarchive.hpp"
#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/filesystem.hpp"
#include <fstream>
#include <curl/curl.h>
#include <string>
#include <algorithm>

using namespace std;

Analytics::Analytics()
{
	// if no file "battle_data.dat" exists, save an empty list to a new file
	if (!boost::filesystem::exists("battle_data.dat")) {
		ofstream new_file("battle_data.dat");
		boost::archive::binary_oarchive archive(new_file);

		archive << battle_entries; // battle_entries are empty at this point

	}
	read_from_binary();
}


Analytics::~Analytics()
{
}

PlayerData Analytics::read_PlayerData(nlohmann::json& data_t)
{

	int kill_count = data_t["kill_count"].get<int>();
	int assist_count = data_t["assist_count"].get<int>();
	int death_count = data_t["death_count"].get<int>();
	int paint_count = data_t["game_paint_point"].get<int>();

	string id = data_t["player"]["principal_id"].get<string>();
	string name = data_t["player"]["nickname"].get<string>();
	int level = data_t["player"]["player_rank"].get<int>();
	int level_prestige = data_t["player"]["star_rank"].get<int>();

	string weapon = data_t["player"]["weapon"]["id"].get<string>();

	// we need to compute the APs for each ability the player is using
	array<string,3> main_abilities = { data_t["player"]["head_skills"]["main"]["id"].get<string>(),
		data_t["player"]["clothes_skills"]["main"]["id"].get<string>(),
		data_t["player"]["shoes_skills"]["main"]["id"].get<string>()
	};

	// it is not guaranteed that all sub abilities are filled, so we iterate here
	vector<string> sub_abilities;
	for (nlohmann::json j : data_t["player"]["head_skills"]["subs"]) {
		sub_abilities.push_back(j["id"].get<string>());
	}
	for (nlohmann::json j : data_t["player"]["clothes_skills"]["subs"]) {
		sub_abilities.push_back(j["id"].get<string>());
	}
	for (nlohmann::json j : data_t["player"]["shoes_skills"]["subs"]) {
		sub_abilities.push_back(j["id"].get<string>());
	}

	// initiate keys in the map
	std::map<std::string, int> abilities;
	for (string k : main_abilities) {
		abilities[k] = 0;
	}
	for (string k : sub_abilities) {
		abilities[k] = 0;
	}

	// fill map with values
	for (string k : main_abilities) {
		abilities[k] += 10;
	}
	for (string k : sub_abilities) {
		abilities[k] += 3;
	}

	PlayerData ret = PlayerData(kill_count, assist_count, death_count, paint_count,
		id, name, level, level_prestige, weapon, abilities);
	return ret;
}

BattleData Analytics::json_to_battle_data(nlohmann::json& data_t, nlohmann::json& details_t, int dc_count_t,
	optional<float>& loss_prediction_t,
	optional<float>& estimated_range_t,
	optional<float>& win_change_t)
{

	// load data that can directly be read
	int is_placements = data_t["x_power"].is_null();
	string game_id = data_t["battle_number"].get<string>();
	float avg_power = data_t["estimate_x_power"].get<float>();
	int match_timestamp = data_t["start_time"].get<int>();
	int match_length = data_t["elapsed_time"].get<int>();
	string game_mode = data_t["rule"]["key"].get<string>();
	bool match_result = (data_t["my_team_result"]["key"].get<string>() == "victory");
	int points_goodguys = data_t["my_team_count"].get<int>();
	int points_badguys = data_t["other_team_count"].get<int>();

	// values that are only valid if it is not a placement match
	std::optional<float> player_xpower;
	std::optional<float> loss_prediction;
	std::optional<float> estimated_range;
	std::optional<float> win_change;

	if (!is_placements) {
		player_xpower = data_t["x_power"].get<int>();
		loss_prediction = loss_prediction_t;
		estimated_range = estimated_range_t;
		win_change = win_change_t;
	}

	// load detailed player data
	std::vector<PlayerData> players_teammates; // player data for all team mates
	std::vector<PlayerData> players_opponents; // player data for all opponents
	PlayerData this_player = read_PlayerData(data_t["player_result"]);
	transform(data_t["my_team_members"].begin(), data_t["my_team_members"].end(), back_inserter(players_teammates),
		[&](nlohmann::json& j) {return read_PlayerData(j); }); // for some reason, I need to wrap the function here
	transform(data_t["other_team_members"].begin(), data_t["other_team_members"].end(), back_inserter(players_opponents), 
		[&](nlohmann::json& j) {return read_PlayerData(j); });


	// infer DC counts from the player data
	int dcs_goodguys = static_cast<int>(
		count_if(players_teammates.begin(), players_teammates.end(), [](PlayerData p) {return p.paint_count == 0; })
		);
	int dcs_badguys = static_cast<int>(
		count_if(players_opponents.begin(), players_opponents.end(), [](PlayerData p) {return p.paint_count == 0; })
		);

	// user's own DC count
	int recent_disconnects = dc_count_t;

	BattleData ret = BattleData(player_xpower, recent_disconnects, is_placements,
		game_id, avg_power, match_timestamp, match_length, game_mode, this_player,
		players_teammates, players_opponents, loss_prediction, estimated_range,
		win_change, match_result, points_goodguys, points_badguys,
		dcs_goodguys, dcs_badguys);
	return ret;
}


nlohmann::json Analytics::load_detail_json(nlohmann::json & data_t, const string& SESSID_t)
{
	nlohmann::json ret;

	string battle_id = data_t["battle_number"].get<string>();
	string req_url = "https://app.splatoon2.nintendo.net/api/results/" + battle_id;
	string json_string = http_requests::load_page(req_url, SESSID_t);

	ret = nlohmann::json::parse(json_string);

	return ret;
}

int Analytics::get_recent_dc_count(const string& SESSID_t) {
	string req_url = "https://app.splatoon2.nintendo.net/api/records";
	string json_string = http_requests::load_page(req_url, SESSID_t);

	nlohmann::json j = nlohmann::json::parse(json_string);

	return j["records"]["recent_disconnect_count"].get<int>();
}


void Analytics::write_to_binary() {
	ofstream file("battle_data.dat");
	boost::archive::binary_oarchive archive(file);

	archive << battle_entries;
}

void Analytics::read_from_binary() {
	ifstream file("battle_data.dat");
	boost::archive::binary_iarchive archive(file);

	archive >> battle_entries;
}


nlohmann::json PlayerData::abilities_to_json() {
	nlohmann::json ret;

	for (auto const& [key, val] : abilities) {
		ret.push_back(nlohmann::json(
			{ {"code", key},
			{"ap", val} }
		));
	}

	return ret;
}


nlohmann::json PlayerData::to_json() {
	nlohmann::json data = {
		{"kill_count", kill_count},
		{"assist_count", assist_count},
		{"death_count", death_count},
		{"paint_count", paint_count},
		{"id", id},
		{"name", "ANONYMIZED"},
		{"level", level},
		{"level_prestige", level_prestige},
		{"weapon", weapon},
		{"abilities", abilities_to_json()}
	};

	return data;
}

template<class T>
nlohmann::json BattleData::optional_to_json(std::optional<T> opt) {
	nlohmann::json ret;
	ret["has_value"] = opt.has_value();
	if (opt.has_value()) {
		//ret["value"] = opt.value();
	}
	
	return ret;
}

nlohmann::json BattleData::to_json() {
	nlohmann::json data = {
		{"player_xpower", optional_to_json(player_xpower)},
		{"recent_disconnects", recent_disconnects},
		{"is_placements", is_placements},
		{"game_id", game_id},
		{"avg_power", avg_power},
		{"match_timestamp", match_timestamp},
		{"match_length", match_length},
		{"game_mode", game_mode},
		{"this_player", this_player.to_json()},
		{"loss_prediction", optional_to_json(loss_prediction)},
		{"estimated_range", optional_to_json(estimated_range)},
		{"win_change", optional_to_json(estimated_range)},
		{"match_result", match_result},
		{"points_goodguys", points_goodguys},
		{"points_badguys", points_badguys},
		{"dcs_goodguys", dcs_goodguys},
		{"dcs_badguys", dcs_badguys}
	};

	//add players_teammates
	nlohmann::json teammates;
	for (PlayerData player : players_teammates) {
		teammates.push_back(player.to_json());
	}
	data["players_teammates"] = teammates;

	//add players_opponents
	nlohmann::json opponents;
	for (PlayerData player : players_opponents) {
		opponents.push_back(player.to_json());
	}
	data["players_opponents"] = opponents;

	return data;
}

UINT add_recent_match_analytics(LPVOID pParam) {
	args_analytics* args = reinterpret_cast<args_analytics*>(pParam);

	// load data json
	string req_url = "https://app.splatoon2.nintendo.net/api/results";
	string SESSID_copy = string(args->SESSID); // we need to use this so args->SESSID doesn't disappear
	string json_string = http_requests::load_page(req_url, SESSID_copy);

	nlohmann::json results_json = nlohmann::json::parse(json_string);
	nlohmann::json prev_match_json = results_json["results"][0];

	nlohmann::json detail_json = args->analytics->load_detail_json(prev_match_json, SESSID_copy);
	int recent_dc_count = args->analytics->get_recent_dc_count(SESSID_copy);

	BattleData new_entry = args->analytics->json_to_battle_data(prev_match_json, detail_json, recent_dc_count,
		args->loss_prediction,
		args->estimated_range,
		args->win_change);

	args->analytics->battle_entries.push_back({ new_entry, false });

	args->analytics->write_to_binary();

	return 0;
}

UINT upload_analytics(LPVOID pParam) {
	//Analytics* analytics = reinterpret_cast<Analytics*>(pParam);

	//for (BattleDataEntry entry : analytics->battle_entries) {
	//	if (!entry.is_uploaded) {
			//entry.battle_data.to_json();
	//	}
	//}
}