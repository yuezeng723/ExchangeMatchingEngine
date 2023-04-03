
#include "insertion.h"
using namespace std;
using namespace pqxx;
#ifndef _IMPLEMENT_
#define _IMPLEMENT_

void doCreate(connection *C, int account_id, double balance, string sym, int shares);

bool orderCheck(connection *C, int account_id, int amount, double limit);

void doOrder(connection *C, int transaction_id, int account_id, string symbol, int amount, double limit);

result orderMatch(connection *C, string symbol, int amount, double limit);
void doQueryOpen(connection *C, int transaction_id);
void doQueryExecute(connection *C, int transaction_id);
void doQueryCancel(connection *C, int transaction_id);
result searchForCancel(connection *C, int transaction_id);
void doCancel(connection *C, int transaction_id, int account_id);

#endif