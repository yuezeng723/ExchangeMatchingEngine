using namespace std;
using namespace pqxx;
#include <ctime>
#ifndef _QUERY_FUNCS_
#define _QUERY_FUNCS_

void addAccount(connection *C, double balance);
void addPosition(connection *C, string symbol);	
void addOrder(connection *C, int account_id, int amount, double limit_price, double execute_price, string status, std::time_t time, int position_id);
bool checkAccount(connection *C, int account_id);
bool checkPosition(connection *C, string symbol);
void updatePosition(connection *C, string symbol, int amount);
#endif