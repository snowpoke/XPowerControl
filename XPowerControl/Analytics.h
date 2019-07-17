// Saves analytics/telemetry about the user and saves it to a data file.
// In a later version, this is also supposed to upload it to my server.

#pragma once
#include "stdafx.h"
#include <string>
#include <optional>
#include <array>
#include "nlohmann/json.hpp"
#include "serialize_std_optional.h"
#include "boost/archive/tmpdir.hpp"
#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/serialization/vector.hpp"
#include "boost/serialization/optional.hpp"
#include "boost/serialization/map.hpp"


struct PlayerData {
	int kill_count; // number of kills, without assists
	int assist_count; // number of assists
	int death_count; // counts how often the player died
	int paint_count; // amount of paint in p

	std::string id; // reported internal user id
	std::string name; // reported user name
	int level; // reported user level number
	int level_prestige; // reported number of how often the player has prestiged

	std::string weapon; // reported weapon id, this is a number, but saved in the JSON as string
	std::map<std::string, int> abilities; // reported ability ids and their AP counts

	template<class Archive>
	void serialize(Archive& ar, const unsigned int file_version) {
		ar & kill_count;
		ar & assist_count;
		ar & death_count;
		ar & paint_count;
		ar & id;
		ar & name;
		ar & level;
		ar & level_prestige;
		ar & weapon;
		ar & abilities;
	}

	// to avoid serialization errors, we require elements to be initialized with all members
	PlayerData(int kill_count, int assist_count, int death_count, int paint_count,
		std::string& id, std::string& name, int level, int level_prestige,
		std::string& weapon, std::map<std::string, int>& abilities) :
			kill_count(kill_count), assist_count(assist_count), death_count(death_count),
			paint_count(paint_count), id(id), name(name), level(level),
			level_prestige(level_prestige), weapon(weapon), abilities(abilities) {}

	PlayerData() {};
};

struct BattleData {
	std::optional<float> player_xpower; // false if placement match, otherwise player's x power at the start of the match
	int recent_disconnects; // number of recent disconnects
	bool is_placements; // is this a placement match?

	std::string game_id; // game id as specified in the NSO app
	float avg_power; // reported average player power in this match
	int match_timestamp; // reported timestamp of the start of the match
	int match_length; // reported length of the match
	std::string game_mode; // reported game mode
	
	PlayerData this_player; // player data for the user of this app
	std::vector<PlayerData> players_teammates; // player data for all team mates
	std::vector<PlayerData> players_opponents; // player data for all opponents

	std::optional<float> loss_prediction; // false if placement match, otherwise power prediction in case of loss
	std::optional<float> estimated_range; // false if placement match, otherwise estimated range before the match started
	std::optional<float> win_change; // false if placement or match was lost, otherwise power that was gained after winning

	bool match_result; // true for victory, false for defeat
	int points_goodguys; // 0-100 points for the user's team
	int points_badguys; // 0-100 points for the opponents' team
	int dcs_goodguys; // number of DCs on the user's team, determined as the number of players with 0p of paint
	int dcs_badguys; // number of DCs on the opponents'team, determined as the number of players with 0p of paint

	template<class Archive>
	void serialize(Archive& ar, const unsigned int file_version) {
		ar & player_xpower;
		ar & recent_disconnects;
		ar & is_placements;
		ar & game_id;
		ar & avg_power;
		ar & match_timestamp;
		ar & match_length;
		ar & game_mode;
		ar & this_player;
		ar & players_teammates;
		ar & players_opponents;
		ar & loss_prediction;
		ar & estimated_range;
		ar & win_change;
		ar & match_result;
		ar & points_goodguys;
		ar & points_badguys;
		ar & dcs_goodguys;
		ar & dcs_badguys;
	}

	// to avoid serialization errors, we require elements to be initialized with all members
	BattleData(std::optional<float> player_xpower, int recent_disconnects, bool is_placements,
		std::string game_id, float avg_power, int match_timestamp, int match_length,
		std::string game_mode, PlayerData this_player, std::vector<PlayerData> players_teammates,
		std::vector<PlayerData> players_opponents, std::optional<float> loss_prediction,
		std::optional<float> estimated_range, std::optional<float> win_change,
		bool match_result, int points_goodguys, int points_badguys,
		int dcs_goodguys, int dcs_badguys) :
			player_xpower(player_xpower), recent_disconnects(recent_disconnects), is_placements(is_placements),
			game_id(game_id), avg_power(avg_power), match_timestamp(match_timestamp), match_length(match_length),
			game_mode(game_mode), this_player(this_player), players_teammates(players_teammates),
			players_opponents(players_opponents), loss_prediction(loss_prediction),
			estimated_range(estimated_range), win_change(win_change),
			match_result(match_result), points_goodguys(points_goodguys), points_badguys(points_badguys),
			dcs_goodguys(dcs_goodguys), dcs_badguys(dcs_badguys) {}

	BattleData() {};
};


// contains the battle data itself and information on whether it has been saved locally and/or uploaded to the server
struct BattleDataEntry {
	BattleData battle_data;
	bool is_uploaded;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int file_version) {
		ar & battle_data;
		ar & is_uploaded;
	}

	BattleDataEntry(BattleData battle_data,  bool is_uploaded) :
		battle_data(battle_data),  is_uploaded(is_uploaded) {}

	BattleDataEntry() {};

};
BOOST_CLASS_VERSION(BattleDataEntry, 1) // set version of the data file so later versions can be easily adapted



class Analytics
{
public:
	Analytics();
	~Analytics();
	
	std::vector<BattleDataEntry> battle_entries; // this vector contains an entry for each battle

	// functions that obtain match data from json files
	PlayerData read_PlayerData(nlohmann::json& data_t);

	BattleData json_to_battle_data(nlohmann::json& data_t, nlohmann::json& details_t, int dc_count_t, 
		std::optional<float>& loss_prediction_t,
		std::optional<float>& estimated_range_t,
		std::optional<float>& win_change_t);

	nlohmann::json load_detail_json(nlohmann::json& data_t, const std::string& SESSID_t);
	int get_recent_dc_count(const std::string & SESSID_t);

	// functions to read and write to local storage
	void write_to_binary();
	void read_from_binary();
};

struct args_analytics {
	Analytics* analytics;
	std::string SESSID;
	std::optional<float> loss_prediction;
	std::optional<float> estimated_range;
	std::optional<float> win_change;
};

UINT add_recent_match_analytics(LPVOID pParam);
