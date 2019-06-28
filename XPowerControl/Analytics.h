#pragma once
#include <string>
#include <optional>
#include <array>
#include "nlohmann/json.hpp"
#include "stdafx.h"


struct APcount {
	std::string ability_id;
	int ap_points;
};

struct PlayerData {
	int kill_count;
	int assist_count;
	int death_count;
	int paint_count;

	std::string id;
	std::string name;
	int level;
	int level_prestige;

	std::string weapon;
	std::map<std::string, int> abilities;
};


struct BattleEntry {
	std::optional<float> player_xpower; // false if placement match
	int recent_disconnects;
	bool is_placements;

	std::string game_id;
	float avg_power;
	int match_timestamp;
	int match_length;
	std::string game_mode;
	
	PlayerData this_player;
	std::vector<PlayerData> players_teammates;
	std::vector<PlayerData> players_opponents;

	std::optional<float> loss_prediction; // false if placement match
	std::optional<float> estimated_range; // false if placement match
	std::optional<float> win_change; // false if placement or match was lost

	bool match_result;
	int points_goodguys;
	int points_badguys;
	int dcs_goodguys;
	int dcs_badguys;
};

class Analytics
{
public:
	Analytics();
	~Analytics();
	
	std::vector<BattleEntry> battles;

	PlayerData read_PlayerData(nlohmann::json& data_t);

	BattleEntry Analytics::json_to_entry(nlohmann::json& data_t, nlohmann::json& details_t, int dc_count_t, 
		std::optional<float>& loss_prediction_t,
		std::optional<float>& estimated_range_t,
		std::optional<float>& win_change_t);

	nlohmann::json load_detail_json(nlohmann::json& data_t, std::string& SESSID_t);
	int get_recent_dc_count(std::string & SESSID_t);
	UINT save_recent_match_analytics(LPVOID pParam);
protected:
	std::vector<BattleEntry> battle_entries;
};

