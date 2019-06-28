#include "Analytics.h"
#include "nlohmann/json.hpp"
#include "monitor.h"
#include "XPowerControlDlg.h"
#include <curl/curl.h>
#include <string>
#include <algorithm>

using namespace std;

Analytics::Analytics()
{
}


Analytics::~Analytics()
{
}

PlayerData Analytics::read_PlayerData(nlohmann::json& data_t)
{
	PlayerData ret;

	ret.kill_count = data_t["kill_count"].get<int>();
	ret.assist_count = data_t["assist_count"].get<int>();
	ret.death_count = data_t["death_count"].get<int>();
	ret.paint_count = data_t["game_paint_point"].get<int>();

	ret.id = data_t["player"]["principal_id"].get<string>();
	ret.name = data_t["player"]["nickname"].get<string>();
	ret.level = data_t["player"]["player_rank"].get<int>();
	ret.level_prestige = data_t["player"]["star_rank"].get<int>();

	ret.weapon = data_t["player"]["weapon"]["id"].get<int>();

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
	for (string k : main_abilities) {
		ret.abilities[k] = 0;
	}
	for (string k : sub_abilities) {
		ret.abilities[k] = 0;
	}

	// fill map with values
	for (string k : main_abilities) {
		ret.abilities[k] += 10;
	}
	for (string k : sub_abilities) {
		ret.abilities[k] += 3;
	}

	return ret;
}

BattleEntry Analytics::json_to_entry(nlohmann::json& data_t, nlohmann::json& details_t, int dc_count_t,
	optional<float>& loss_prediction_t,
	optional<float>& estimated_range_t,
	optional<float>& win_change_t)
{
	BattleEntry ret;

	// load data that can directly be read
	ret.is_placements = data_t["x_power"].is_null();
	ret.game_id = data_t["battle_number"].get<string>();
	ret.avg_power = data_t["estimate_x_power"].get<int>();
	ret.match_timestamp = data_t["start_time"].get<int>();
	ret.match_length = data_t["elapsed_time"].get<int>();
	ret.game_mode = data_t["rule"]["key"].get<string>();
	ret.match_result = (data_t["my_team_result"].get<string>() == "victory");
	ret.points_goodguys = data_t["my_team_count"].get<int>();
	ret.points_badguys = data_t["other_team_count"].get<int>();

	// values that are only valid if it is not a placement match
	if (ret.is_placements) {
		ret.player_xpower = data_t["x_power"].get<int>();
		ret.loss_prediction = loss_prediction_t;
		ret.estimated_range = estimated_range_t;
		ret.win_change = win_change_t;
	}

	// load detailed player data
	ret.this_player = read_PlayerData(data_t["player_result"]);
	transform(data_t["my_team_members"].begin(), data_t["my_team_members"].end(), back_inserter(ret.players_teammates),
		[&](nlohmann::json& j) {return read_PlayerData(j); }); // for some reason, I need to wrap the function here
	transform(data_t["other_team_members"].begin(), data_t["other_team_members"].end(), back_inserter(ret.players_opponents), 
		[&](nlohmann::json& j) {return read_PlayerData(j); });


	// infer DC counts from the player data
	ret.dcs_goodguys = count_if(ret.players_teammates.begin(), ret.players_teammates.end(), [](PlayerData p) {return p.paint_count == 0; });
	ret.dcs_badguys = count_if(ret.players_opponents.begin(), ret.players_opponents.end(), [](PlayerData p) {return p.paint_count == 0; });

	// user's own DC count
	ret.recent_disconnects = dc_count_t;
	return ret;
}


nlohmann::json Analytics::load_detail_json(nlohmann::json & data_t, string& SESSID_t)
{
	nlohmann::json ret;

	string battle_id = data_t["battle_number"].get<string>();
	string req_url = "https://app.splatoon2.nintendo.net/api/results/" + battle_id;
	string json_string = loadPage(req_url, SESSID_t);

	ret = nlohmann::json::parse(json_string);

	return ret;
}

int Analytics::get_recent_dc_count(string& SESSID_t) {
	string req_url = "https://app.splatoon2.nintendo.net/api/records";
	string json_string = loadPage(req_url, SESSID_t);

	nlohmann::json j = nlohmann::json::parse(json_string);

	return j["records"]["recent_disconnect_count"].get<int>();
}

struct args_analytics {
	string SESSID;
	optional<float> loss_prediction;
	optional<float> estimated_range;
	optional<float> win_change;
};

UINT Analytics::save_recent_match_analytics(LPVOID pParam) {
	args_analytics* args = reinterpret_cast<args_analytics*>(pParam);

	// load data json
	string req_url = "https://app.splatoon2.nintendo.net/api/results/";
	string json_string = loadPage(req_url, args->SESSID);

	nlohmann::json results_json = nlohmann::json::parse(json_string);
	nlohmann::json prev_match_json = results_json["results"][0];

	nlohmann::json detail_json = load_detail_json(prev_match_json, args->SESSID);
	int dc_count = get_recent_dc_count(args->SESSID);

	BattleEntry new_entry = json_to_entry(prev_match_json, detail_json, dc_count,
		args->loss_prediction,
		args->estimated_range,
		args->win_change);

	battle_entries.push_back(new_entry);
	return 0;
}