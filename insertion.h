using namespace std;
using namespace pqxx;
#include <ctime>
#ifndef _QUERY_FUNCS_
#define _QUERY_FUNCS_

void addAccount(connection *C, int account_id, double balance, int position_id);
void addPosition(connection *C, string symbol, int shares);
void addOpenOrder(connection *C, int transaction_id, int shares, double limit_price, string symbol);
void addExecuteOrder(connection *C, int transaction_id, int shares, std::time_t time, double execute_price);
void addCancelOrder(connection *C, int transaction_id, int shares, std::time_t time);
void deleteOpenOrder(connection *C, int open_id);
void updateOpenOrder(connection *C, int open_id, int shares);
bool checkPosition(connection *C, string symbol);
#endif