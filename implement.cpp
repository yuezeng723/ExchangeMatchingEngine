
bool orderCheck(connection *C, int account_id, int amount, double limit) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT ACCOUNT.balance FROM ACCOUNT WHERE account_id=" << account_id << ";";
  result R( N.exec(sql));
  result::const_iterator c = R.begin();
  if(c[0].asint<>() < amount * limit) return true;
  return false;
}
void doOrder(connection *C, int transaction_id, int account_id, string symbol, int amount, double limit) {
  if(orderCheck(*C, account_id, amount, limit)) return;
  result R = orderMatch(C, symbol, amount, limit);
  
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    if(amount != 0) {
      int shares = c[2].as<int>();
      if(abs(shares) < abs(amount)) {
        executeOrder(C, c[1].as<int>(), shares);
        addOrder(C, transaction_id, "executed", -shares, std::time(nullptr), limit,\
         c[4].as<double>(), symbol);
        addOrder(C, transaction_id, "open", amount+shares, std::time(nullptr), limit,\
         limit, symbol);
      } else if(abs(shares) > abs(amount)) {
        executeOrder(C, c[1].as<int>(), -amount);
        addOrder(C, c[0].as<int>(), "open", shares + amount, std::time(nullptr), limit,\
         limit, symbol);
        addOrder(C, transaction_id, "executed", amount, std::time(nullptr), limit,\
         c[4].as<double>(), symbol);
      } else {
        executeOrder(C, c[1].as<int>(), shares);
        addOrder(C, transaction_id, "executed", amount, std::time(nullptr), limit,\
         c[4].as<double>(), symbol);
      }
    }
  }
  addOrder(C, transaction_id, status, shares, time, limit_price, execute_price, symbol);
}
void executeOrder(connection *C, int order_id, int shares) {
    work W(*C);
    stringstream query;
    query << "UPDATE BANK_ORDER SET status=" << W.quote("executed") << ", shares=" << shares\
    << " WHERE order_id=" << order_id << ";";
    W.exec(sql.str());
    W.commit();
}
result orderMatch(connection *C, string symbol, int amount, double limit)
{
  nontransaction N(*C);
  stringstream sql;
  string op = "open";
  if(amount < 0) {
    sql << "SELECT transaction_id, BANK_ORDER.order_id, BANK_ORDER.shares, BANK_ORDER.limit_price FROM BANK_ORDER,ACCOUNT,POSITION WHERE \
    POSITION.symbol=" << symbol << " AND BANK_ORDER.status=" << N.quote(op) <<" AND BANK_ORDER.share>0 AND BANK_ORDER.limit_price>" << limit << \
    "ORDER BY BANK_ORDER.limit_price ASC;";
  } else {
      sql << "SELECT transaction_id, BANK_ORDER.order_id, BANK_ORDER.shares, BANK_ORDER.limit_price FROM BANK_ORDER,ACCOUNT,POSITION WHERE \
      POSITION.symbol=" << symbol << " AND BANK_ORDER.status=" << N.quote(op) <<" AND BANK_ORDER.share<0 AND BANK_ORDER.limit_price<" << limit << \
      "ORDER BY limit_price ASC, order_id ASC;";
  }
  result R( N.exec(sql));
  return R;
}

void doQuery(connection *C, int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT BANK_ORDER.shares, BANK_ORDER.execute_price, BANK_ORDER.time FROM \
  BANK_ORDER WHERE BANK_ORDER.transaction_id=" << transaction_id << ";";
  result R( N.exec(sql));
  return R;
}
void doCancel(connection *C, int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT BANK_ORDER.order_id,BANK_ORDER.shares,BANK_ORDER.time FROM BANK_ORDER WHERE BANK_ORDER.transaction_id=" \
  << transaction_id << " AND BANK_ORDER.status=" << N.quote("open") << ";";
  result R( N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    work W(*C);
    stringstream query;
    query << "UPDATE BANK_ORDER SET status=" << W.quote("canceled") << " WHERE order_id =" << c[0].asint<>() << ";";
    W.exec(sql.str());
    W.commit();
  }
  return R;
}
