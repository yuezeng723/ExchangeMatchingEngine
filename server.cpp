#include "server.hpp"

void Server::createTables() {
  work W(*C);

  string accountSQL =
    "CREATE TABLE ACCOUNT"
    "(account_id SERIAL PRIMARY KEY,"
    "balance DECIMAL(10,2) NOT NULL);"; 

  string positionSQL =
    "CREATE TABLE POSITION"
    "(position_id SERIAL PRIMARY KEY,"
    "symbol VARCHAR(20) NOT NULL,"
    "account_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "FOREIGN KEY(account_id) REFERENCES ACCOUNT(account_id)"
    "ON DELETE SET NULL ON UPDATE CASCADE);"; 

  string openSQL =
    "CREATE TABLE OPENORDER"
    "(open_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "limit_price DECIMAL(10,2),"
    "symbol VARCHAR(20) NOT NULL);";
  string executeSQL =
    "CREATE TABLE EXECUTEORDER"
    "(execute_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "time TIME,"
    "execute_price DECIMAL(10,2));";
  string cancelSQL =
    "CREATE TABLE CANCELORDER"
    "(cancel_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "time TIME);";

  
  W.exec(accountSQL);
  W.exec(positionSQL);
  W.exec(openSQL);
  W.exec(executeSQL);
  W.exec(cancelSQL);
  W.commit();
}

void Server::initialDatabase() {
  try{
    C = new connection("dbname=exchange_matching user=postgres password=ece568");
    if (C->is_open()) {
      // cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
  }

  try{
    string dropSql = "DROP TABLE IF EXISTS POSITION, ACCOUNT, openorder, executeorder, cancelorder;";
    /* Create a transactional object. */
    work W(*C);
    /* Execute drop */
    W.exec(dropSql);
    W.commit();
    handler = new sqlHandler(C);
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
  }
}

void Server::doCreate(int account_id, double balance, string sym, int shares) {
  handler->addAccount(account_id, balance);
  handler->addPosition(sym, account_id, shares);
}
// only check for buy right now
bool Server::orderCheck(int account_id, int amount, double limit) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT ACCOUNT.balance FROM ACCOUNT WHERE account_id=" << account_id << ";";
  result R( N.exec(sql));
  result::const_iterator c = R.begin();
  if(c[0].as<int>() < amount * limit || c == R.end()) return true;
  return false;
} //+-

void Server::doOrder(int transaction_id, int account_id, string symbol, int amount, double limit) {
  // if(orderCheck(*C, account_id, amount, limit)) return; //change!!
//reduce account balance
  //updateAccount need to be done after checking account valid
  handler->updateAccount(account_id, -amount*limit);
  result R = orderMatch(symbol, amount, limit);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    if(amount != 0) {
      int shares = c[2].as<int>();
      if(abs(shares) < abs(amount)) {
        handler->addExecuteOrder(c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        handler->deleteOpenOrder(c[1].as<int>());
        handler->addExecuteOrder(transaction_id, -shares, std::time(nullptr), c[3].as<double>());
        handler->updatePosition(symbol, account_id, -shares);
        amount += shares;
      } else if(abs(shares) > abs(amount)) {
        handler->addExecuteOrder(c[0].as<int>(), -amount, std::time(nullptr), c[3].as<double>());
        handler->addExecuteOrder(transaction_id, amount, std::time(nullptr), c[3].as<double>());
        handler->updateOpenOrder(c[1].as<int>(), amount+shares);
        handler->updatePosition(symbol, account_id, amount);
        amount = 0;
      } else {
        handler->addExecuteOrder(c[0].as<int>(), shares, std::time(nullptr), c[3].as<double>());
        handler->deleteOpenOrder(c[1].as<int>());
        handler->addExecuteOrder(transaction_id, amount, std::time(nullptr), c[3].as<double>());
        handler->updatePosition(symbol, account_id, amount);
        amount = 0;
      }
    }
  }
  if(amount != 0) handler->addOpenOrder(transaction_id, amount, limit, symbol);
}
result Server::orderMatch(string symbol, int amount, double limit)
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

void Server::doQueryOpen(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << endl;
  }
}
void Server::doQueryExecute(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, execute_price, time FROM EXECUTEORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << ", " << c[1].as<string>() << endl;
    // cout << c[0].as<string>() << endl;
  }
}
void Server::doQueryCancel(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, time FROM CANCELORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << transaction_id << "," << c[0].as<string>() << ", " << c[1].as<string>() << endl;
    // cout << c[0].as<string>() << endl;
  }
}
result Server::searchForCancel(int transaction_id) {
    nontransaction N(*C);
    stringstream sql;
    sql << "SELECT open_id, shares, limit_price, symbol FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
    result R( N.exec(sql));
    return R;
}
void Server::doCancel(int transaction_id, int account_id) { //cancel shares combine
  result R = searchForCancel(transaction_id);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    handler->updateAccount(account_id, c[1].as<int>()*c[2].as<double>());
    handler->addCancelOrder(transaction_id, c[1].as<int>(), std::time(nullptr));
    handler->deleteOpenOrder(c[0].as<int>());
  }
  //add update account
}

void Server::handleClient(int client_fd) {
    char buffer[4000];
    int bytesReceived;
    while ((bytesReceived = recv(client_fd, buffer, 4000, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        const char* response = "server get";
        send(client_fd, response, strlen(response), 0);
    }
    close(client_fd);
}