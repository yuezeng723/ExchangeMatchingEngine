#include "implement.h"
using namespace std;

// only check for buy right now
bool orderCheck(connection *C, int account_id, int amount, double limit) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT ACCOUNT.balance FROM ACCOUNT WHERE account_id=" << account_id << ";";
  result R( N.exec(sql));
  result::const_iterator c = R.begin();
  if(c[0].as<int>() < amount * limit || c == R.end()) return true;
  return false;
} //+-

void doOrder(connection *C, int transaction_id, int account_id, string symbol, int amount, double limit) {
  // if(orderCheck(*C, account_id, amount, limit)) return; //change!!
//reduce account balance
  //updateAccount need to be done after checking account valid
  updateAccount(C, account_id, -amount*limit);
  result R = orderMatch(C, symbol, amount, limit);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    if(amount != 0) {
      int shares = c[2].as<int>();
      if(abs(shares) < abs(amount)) {
        addExecuteOrder(C, c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        deleteOpenOrder(C, c[1].as<int>());
        addExecuteOrder(C, transaction_id, -shares, std::time(nullptr), c[3].as<double>());
        updatePosition(C, symbol, account_id, -shares);
        amount += shares;
      } else if(abs(shares) > abs(amount)) {
        addExecuteOrder(C, c[0].as<int>(), -amount, std::time(nullptr), c[3].as<double>());
        addExecuteOrder(C, transaction_id, amount, std::time(nullptr), c[3].as<double>());
        updateOpenOrder(C, c[1].as<int>(), amount+shares);
        updatePosition(C, symbol, account_id, amount);
        amount = 0;
      } else {
        addExecuteOrder(C, c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        deleteOpenOrder(C, c[1].as<int>());
        addExecuteOrder(C, transaction_id, amount, std::time(nullptr), c[3].as<double>());
        updatePosition(C, symbol, account_id, amount);
        amount = 0;
      }
    }
  }
  if(amount != 0) addOpenOrder(C, transaction_id, amount, limit, symbol);
}
result orderMatch(connection *C, string symbol, int amount, double limit)
{
  nontransaction N(*C);
  stringstream sql;
  string op = "open";
  if(amount < 0) {
    sql << "SELECT transaction_id, open_id, shares, limit_price FROM OPENORDER WHERE \
    symbol=" << N.quote(symbol) << " AND shares>0 AND limit_price>" << limit << "ORDER BY limit_price DESC, open_id ASC;";
  } else {
    sql << "SELECT transaction_id, open_id, shares, limit_price FROM OPENORDER WHERE \
    symbol=" << N.quote(symbol) << " AND shares<0 AND limit_price<" << limit << "ORDER BY limit_price ASC, open_id ASC;";
  }
  result R( N.exec(sql));
  return R;
}

void doQueryOpen(connection *C, int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << endl;
  }
}
void doQueryExecute(connection *C, int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, execute_price, time FROM EXECUTEORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << ", " << c[1].as<string>() << endl;
    // cout << c[0].as<string>() << endl;
  }
}
void doQueryCancel(connection *C, int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, time FROM CANCELORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << ", " << c[1].as<string>() << endl;
    // cout << c[0].as<string>() << endl;
  }
}
result searchForCancel(connection *C, int transaction_id) {
    nontransaction N(*C);
    stringstream sql;
    sql << "SELECT open_id, shares, limit_price, symbol FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
    result R( N.exec(sql));
    return R;
}
void doCancel(connection *C, int transaction_id, int account_id) { //cancel shares combine
  result R = searchForCancel(C, transaction_id);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    updateAccount(C, account_id, c[1].as<int>()*c[2].as<double>());
    addCancelOrder(C, transaction_id, c[1].as<int>(), std::time(nullptr));
    deleteOpenOrder(C, c[0].as<int>());
  }
  //add update account
}
