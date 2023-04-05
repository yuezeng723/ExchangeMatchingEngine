#include "sqlHandler.hpp"

void sqlHandler::addAccount(int account_id, double balance) {  
    stringstream res;  
    try {
        stringstream sql;
        work W(*C);
        sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance <<");";
        result R = W.exec(sql.str());
        W.commit();
    } catch (const std::exception& e) {
        res << "error: " << e.what() << std::endl;
    }

}
void sqlHandler::updateAccount(int account_id, double addon) {    
    stringstream sql;
    work W(*C);
    sql << "UPDATE ACCOUNT SET balance=balance+" << addon << " WHERE account_id=" << account_id << ";";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addPosition(string symbol, int account_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO POSITION (symbol, account_id, shares) VALUES (" << W.quote(symbol) <<\
    "," << account_id << "," << shares << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::updatePosition(string symbol, int account_id, int shares) { //mutex lock
    stringstream sql;
    work W(*C);
    sql << "UPDATE POSITION SET shares=shares+" << shares << " WHERE account_id=" << account_id <<\
    " AND symbol=" << W.quote(symbol) << ";";
    result R = W.exec(sql.str());
    if(R.affected_rows() == 0) {
        W.abort();
        addPosition(symbol, account_id, shares);
    }
    else W.commit();
}
void sqlHandler::addOpenOrder(int transaction_id, int shares, double limit_price, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO openorder (transaction_id, shares, limit_price, symbol) VALUES (" 
    << transaction_id << "," << shares << "," << limit_price << "," << W.quote(symbol) << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addExecuteOrder(int transaction_id, int shares, std::time_t time, double execute_price) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO executeorder (transaction_id, shares, time, execute_price) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << "), " << execute_price << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addCancelOrder(int transaction_id, int shares, std::time_t time) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO cancelorder (transaction_id, shares, time) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << ")" << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::deleteOpenOrder(int open_id) {
    stringstream sql;
    work W(*C);
    sql << "DELETE FROM OPENORDER WHERE open_id=" << open_id << ";"; 
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::updateOpenOrder(int open_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "UPDATE OPENORDER SET shares=shares+" << shares << " WHERE open_id=" << open_id << ";";
    W.exec(sql.str());
    W.commit();
}
int sqlHandler::addTransaction() {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO transaction DEFAULT VALUES RETURNING id;";
    result res = W.exec(sql.str());
    int id = res[0]["id"].as<int>();
    W.commit();
    return id;
}

// check if the account exists, return true if exists
bool sqlHandler::checkAccountExist(int account_id) {
    stringstream sql;
    work W(*C);
    sql << "SELECT * FROM ACCOUNT WHERE account_id=" << account_id << ";";
    result R = W.exec(sql.str());
    if(R.size() == 0) {
        W.abort();
        return false;
    }
    else {
        W.commit();
        return true;
    }
}
// check if the position exists, return true if exists
bool sqlHandler::checkPositionExist(string symbol, int account_id) {
    stringstream sql;
    work W(*C);
    sql << "SELECT * FROM POSITION WHERE account_id=" << account_id << " AND symbol=" << W.quote(symbol) << ";";
    result R = W.exec(sql.str());
    if(R.size() == 0) {
        W.abort();
        return false;
    }
    else {
        W.commit();
        return true;
    }
}

bool sqlHandler::checkOpenOrderExist(int transaction_id) {
    stringstream sql;
    work W(*C);
    sql << "SELECT * FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
    result R = W.exec(sql.str());
    if(R.size() == 0) {
        W.abort();
        return false;
    }
    else {
        W.commit();
        return true;
    }
}

// check whether the account has sufficient balance for buy order
bool sqlHandler::checkValidBuyOrder(int account_id, int amount, double limit) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT balance FROM ACCOUNT WHERE account_id=" << account_id << ";";
  result R(N.exec(sql));
  if (R.empty()) {return false;}
  result::const_iterator c = R.begin();
  if (c[0].as<double>() >= amount * limit) {return true;}
  return false;
} 
//check whether the account has sufficient shares for sell order
bool sqlHandler::checkValidSellOrder(int account_id, string symbol, int amount) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares FROM POSITION WHERE account_id=" << account_id << " AND symbol=" << N.quote(symbol) << ";";
  result R(N.exec(sql));
  if (R.empty()) {return false;}
  result::const_iterator c = R.begin();
  if (c[0].as<int>() >= amount) {return true;}
  return false;
}

int sqlHandler::doOrder(int account_id, string symbol, int amount, double limit) {
  updateAccount(account_id, -amount*limit);
  result R = orderMatch(symbol, amount, limit);
  int transaction_id = addTransaction();
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    if(amount != 0) {
      int shares = c[2].as<int>();
      if(abs(shares) < abs(amount)) {
        addExecuteOrder(c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        deleteOpenOrder(c[1].as<int>());
        addExecuteOrder(transaction_id, -shares, std::time(nullptr), c[3].as<double>());
        updatePosition(symbol, account_id, -shares);
        amount += shares;
      } else if(abs(shares) > abs(amount)) {
        addExecuteOrder(c[0].as<int>(), -amount, std::time(nullptr), c[3].as<double>());
        addExecuteOrder(transaction_id, amount, std::time(nullptr), c[3].as<double>());
        updateOpenOrder(c[1].as<int>(), amount);
        updatePosition(symbol, account_id, amount);
        amount = 0;
      } else {
        addExecuteOrder(c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        deleteOpenOrder(c[1].as<int>());
        addExecuteOrder(transaction_id, amount, std::time(nullptr), c[3].as<double>());
        updatePosition(symbol, account_id, amount);
        amount = 0;
      }
    }
  }
  if(amount != 0) addOpenOrder(transaction_id, amount, limit, symbol);
  return transaction_id;
}

//match the order
result sqlHandler::orderMatch(string symbol, int amount, double limit) {
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

//query the open order with transaction_id, and return the dataset
result sqlHandler::doQueryOpen(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}

result sqlHandler::doQueryExecute(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, execute_price, time FROM EXECUTEORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}
result sqlHandler::doQueryCancel(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, time FROM CANCELORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}

result sqlHandler::searchForCancel(int transaction_id) {
    nontransaction N(*C);
    stringstream sql;
    sql << "SELECT open_id, shares, limit_price, symbol FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
    result R( N.exec(sql));
    return R;
}

//cancel the order with transaction_id and account_id
void sqlHandler::doCancel(int transaction_id, int account_id) { //cancel shares combine
  result R = searchForCancel(transaction_id);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    updateAccount(account_id, c[1].as<int>()*c[2].as<double>());
    addCancelOrder(transaction_id, c[1].as<int>(), std::time(nullptr));
    deleteOpenOrder(c[0].as<int>());
  }
}
