using namespace std;
using namespace pqxx;
#include <ctime>
#ifndef _QUERY_FUNCS_
#define _QUERY_FUNCS_

void addAccount(connection *C, int team_id, int jersey_num, string first_name, string last_name,
		int mpg, int ppg, int rpg, int apg, double spg, double bpg);

void add_team(connection *C, string name, int state_id, int color_id, int wins, int losses);

void add_state(connection *C, string name);

void add_color(connection *C, string name);