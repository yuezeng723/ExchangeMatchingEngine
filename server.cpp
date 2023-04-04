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

  string transactionSQL = 
    "CREATE TABLE TRANSACTION"
    "(id SERIAL PRIMARY KEY);";
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
  W.exec(transactionSQL);
  W.exec(openSQL);
  W.exec(executeSQL);
  W.exec(cancelSQL);
  W.commit();
}

void Server::initialDatabase() {
  try{
    C = new connection("dbname=exchange_matching user=postgres password=ece568");
    C->is_open();
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
  }

  try{
    string dropSql = "DROP TABLE IF EXISTS POSITION, ACCOUNT, transaction, openorder, executeorder, cancelorder;";
    /* Create a transactional object. */
    work W(*C);
    /* Execute drop */
    W.exec(dropSql);
    W.commit();
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
  }
}

// check whether the account has sufficient balance for buy order
bool Server::checkValidBuyOrder(int account_id, int amount, double limit) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT ACCOUNT.balance FROM ACCOUNT WHERE account_id=" << account_id << ";";
  result R( N.exec(sql));
  result::const_iterator c = R.begin();
  if(c[0].as<int>() < amount * limit || c == R.end()) return true;
  return false;
} 
//check whether the account has sufficient shares for sell order
bool Server::checkValidSellOrder(int account_id, string symbol, int amount) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT POSITION.shares FROM POSITION WHERE account_id=" << account_id << " AND symbol='" << symbol << "';";
  result R( N.exec(sql));
  result::const_iterator c = R.begin();
  if(c[0].as<int>() < amount || c == R.end()) return true;
  return false;
}
//execute the order
int Server::doOrder(int account_id, string symbol, int amount, double limit) {
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
//query the open order with transaction_id, and return the dataset
result Server::doQueryOpen(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}
result Server::doQueryExecute(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, execute_price, time FROM EXECUTEORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}
result Server::doQueryCancel(int transaction_id) {
  nontransaction N(*C);
  stringstream sql;
  sql << "SELECT shares, time FROM CANCELORDER WHERE transaction_id=" << transaction_id << ";";
  result R(N.exec(sql));
  return R;
}


result Server::searchForCancel(int transaction_id) {
    nontransaction N(*C);
    stringstream sql;
    sql << "SELECT open_id, shares, limit_price, symbol FROM OPENORDER WHERE transaction_id=" << transaction_id << ";";
    result R( N.exec(sql));
    return R;
}

//cancel the order with transaction_id and account_id
void Server::doCancel(int transaction_id, int account_id) { //cancel shares combine
  result R = searchForCancel(transaction_id);
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    updateAccount(account_id, c[1].as<int>()*c[2].as<double>());
    addCancelOrder(transaction_id, c[1].as<int>(), std::time(nullptr));
    deleteOpenOrder(c[0].as<int>());
  }
}
// check if the account exists, return true if exists
bool Server::checkAccountExist(int account_id) {
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
bool Server::checkPositionExist(string symbol, int account_id) {
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

bool Server::checkOpenOrderExist(int transaction_id) {
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

int Server::addTransaction() {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO transaction DEFAULT VALUES RETURNING id;";
    result res = W.exec(sql.str());
    int id = res[0]["id"].as<int>();
    W.commit();
    return id;
}
void Server::addAccount(int account_id, double balance) {  
    stringstream res;  
    try {
        stringstream sql;
        work W(*C);
        sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance <<");";
        result R = W.exec(sql.str());
        W.commit();
    } catch (const std::exception& e) {
        res << "error: " << e.what() << std::endl;
        // return 1;
    }

}
void Server::updateAccount(int account_id, double addon) {    
    stringstream sql;
    work W(*C);
    sql << "UPDATE ACCOUNT SET balance=balance+" << addon << " WHERE account_id=" << account_id << ";";
    W.exec(sql.str());
    W.commit();
}
void Server::addPosition(string symbol, int account_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO POSITION (symbol, account_id, shares) VALUES (" << W.quote(symbol) <<\
    "," << account_id << "," << shares << ");";
    W.exec(sql.str());
    W.commit();
}
void Server::updatePosition(string symbol, int account_id, int shares) { //mutex lock
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
void Server::addOpenOrder(int transaction_id, int shares, double limit_price, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO openorder (transaction_id, shares, limit_price, symbol) VALUES (" 
    << transaction_id << "," << shares << "," << limit_price << "," << W.quote(symbol) << ");";
    W.exec(sql.str());
    W.commit();
}
void Server::addExecuteOrder(int transaction_id, int shares, std::time_t time, double execute_price) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO executeorder (transaction_id, shares, time, execute_price) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << "), " << execute_price << ");";
    W.exec(sql.str());
    W.commit();
}
void Server::addCancelOrder(int transaction_id, int shares, std::time_t time) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO cancelorder (transaction_id, shares, time) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << ")" << ");";
    W.exec(sql.str());
    W.commit();
}
void Server::deleteOpenOrder(int open_id) {
    stringstream sql;
    work W(*C);
    sql << "DELETE FROM OPENORDER WHERE open_id=" << open_id << ";"; 
    W.exec(sql.str());
    W.commit();
}
void Server::updateOpenOrder(int open_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "UPDATE OPENORDER SET shares=shares+" << shares << " WHERE open_id=" << open_id << ";";
    W.exec(sql.str());
    W.commit();
}

void Server::handleClient(int client_fd) {
    char buffer[4000];
    int bytesReceived;
    while ((bytesReceived = recv(client_fd, buffer, 4000, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        string response = "";
        parseBuffer(buffer, bytesReceived, response);

        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
}

void Server::parseBuffer(char* buffer, int size, string &response) {
  cout << "the request size is: " << size << endl;
  stringstream bufferStream;
  bufferStream.write(buffer, size);
  string firstLine;
  bufferStream >> firstLine;
  string content;
  getline(bufferStream, content, '\0');
  int content_size;
  content_size = stoi(firstLine);
  // Check if the content_size is valid
  if (content_size < 0 || content_size > size) {
    throw runtime_error("Invalid content size");
  }
  // Read the XML content directly from the bufferStream
  pt::ptree root;
  stringstream contentStream(content);
  pt::read_xml(contentStream, root);

  string rootTag = root.begin()->first;
  if (rootTag == "create") {
    handleCreate(root, response);
  } else if (rootTag == "transactions") {
    handleTransaction(root, response);
  } else {
    throw runtime_error("Invalid XML file in parseBuffer");
  }
}

string Server::handleCreate(pt::ptree &root, string &response){
  pt::ptree tree;
  pt::ptree& treeRoot = tree.add("result", "");
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("create")) {
    if (v.first == "account") {
      int account_id = v.second.get<int>("<xmlattr>.id");
      double balance = v.second.get<double>("<xmlattr>.balance");
      if (checkAccountExist(account_id)) {
        pt::ptree &error = treeRoot.add("error", "Account already exists");
        error.put("<xmlattr>.id", account_id);
      } else {
        addAccount(account_id, balance);
        pt::ptree &created = treeRoot.add("created", "");
        created.put("<xmlattr>.id", account_id);
      }
    } else if (v.first == "symbol") {
      string symbol = v.second.get<string>("<xmlattr>.sym");
      int curr_account_id = v.second.get_child("account").get<int>("<xmlattr>.id");
      if (checkAccountExist(curr_account_id)) {
        int amount = root.get<int>("create.symbol.account");
        if (checkPositionExist(symbol, curr_account_id)) {
          updatePosition(symbol, curr_account_id, amount);
        } else {
          addPosition(symbol, curr_account_id, amount);
        }
        pt::ptree &created = treeRoot.add("created", "");
        created.put("<xmlattr>.sym", symbol);
        created.put("<xmlattr>.id", curr_account_id);
      } else {
        pt::ptree &error = treeRoot.add("error", "Account does not exist");
        error.put("<xmlattr>.sym", symbol);
        error.put("<xmlattr>.id", curr_account_id);
      }
    }
  }
  stringstream xmlOutput;
  pt::write_xml(xmlOutput, tree);
  response = xmlOutput.str();
  return response;
}


string Server::handleTransaction(pt::ptree &root, string &response){
  pt::ptree tree;
  pt::ptree& treeRoot = tree.add("result", "");
  int account_id = root.get<int>("transactions.<xmlattr>.id");
  stringstream xmlOutput;
  if (!checkAccountExist(account_id)) {
    pt::ptree &error = treeRoot.add("error", "Account does not exist");
    error.put("<xmlattr>.id", account_id);
    pt::write_xml(xmlOutput, tree);
    response = xmlOutput.str();
    return response;
  }
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("transaction")) {
    if (v.first == "order") {
      string symbol = v.second.get<string>("<xmlattr>.sym");
      int amount = v.second.get<int>("<xmlattr>.amount");
      double limit_price = v.second.get<double>("<xmlattr>.limit");
      if (amount >= 0 && !checkValidBuyOrder(account_id, amount, limit_price)) {
        pt::ptree &error = treeRoot.add("error", "Insufficient balance");
        error.put("<xmlattr>.sym", symbol);
        error.put("<xmlattr>.amount", amount);
        error.put("<xmlattr>.limit", limit_price);
      }
      else if (amount < 0 && !checkValidSellOrder(account_id, symbol, amount)) {
        pt::ptree &error = treeRoot.add("error", "Insufficient shares");
        error.put("<xmlattr>.sym", symbol);
        error.put("<xmlattr>.amount", amount);
        error.put("<xmlattr>.limit", limit_price);
      }
      else {
        int transaction_id = doOrder(account_id, symbol, amount, limit_price);
        pt::ptree &opened = treeRoot.add("opened", "");
        opened.put("<xmlattr>.sym", symbol);
        opened.put("<xmlattr>.amount", amount);
        opened.put("<xmlattr>.limit", limit_price);
        opened.put("<xmlattr>.id", transaction_id);
      }
    } 
    else if (v.first == "query") {
      int transaction_id = v.second.get<int>("<xmlattr>.id");
      result openedOrders = doQueryOpen(transaction_id);
      result executedOrders = doQueryExecute(transaction_id);
      result canceledOrders = doQueryCancel(transaction_id);
      pt::ptree &status = treeRoot.add("status", "");
      status.put("<xmlattr>.id", transaction_id);
      if (!openedOrders.empty()) {
        for (const auto &order : openedOrders) {
          pt::ptree &status_open = status.add("open", "");
          status_open.put("<xmlattr>.shares", order["shares"].as<int>());
        }
      }
      if (!canceledOrders.empty()) {
        for (const auto &order : canceledOrders) {
          pt::ptree &status_cancel = status.add("canceled", "");
          status_cancel.put("<xmlattr>.shares", order["shares"].as<int>());
          status_cancel.put("<xmlattr>.time", order["time"].as<string>());
        }
      }
      if (!executedOrders.empty()) {
        for (const auto &order : executedOrders) {
          pt::ptree &status_exec = status.add("executed", "");
          status_exec.put("<xmlattr>.shares", order["shares"].as<int>());
          status_exec.put("<xmlattr>.price", order["execute_price"].as<double>());
          status_exec.put("<xmlattr>.time", order["time"].as<string>());
        }
      }
    }
    else if (v.first == "cancel") {
      int transaction_id = v.second.get<int>("<xmlattr>.id");
      if (!checkOpenOrderExist(transaction_id)) {
        pt::ptree &error = treeRoot.add("error", "No open order for canellation");
        error.put("<xmlattr>.id", transaction_id);
      } else {
        doCancel(transaction_id, account_id);
        pt::ptree &canceled = treeRoot.add("canceled", "");
        canceled.put("<xmlattr>.id", transaction_id);
        result canceledOrders = doQueryCancel(transaction_id);
        for (const auto &order : canceledOrders) {
          pt::ptree &status_cancel = canceled.add("canceled", "");
          status_cancel.put("<xmlattr>.shares", order["shares"].as<int>());
          status_cancel.put("<xmlattr>.time", order["time"].as<string>());
        }
        result executedOrders = doQueryExecute(transaction_id);
        for (const auto &order : executedOrders) {
          pt::ptree &status_exec = canceled.add("executed", "");
          status_exec.put("<xmlattr>.shares", order["shares"].as<int>());
          status_exec.put("<xmlattr>.price", order["execute_price"].as<double>());
          status_exec.put("<xmlattr>.time", order["time"].as<string>());
        }
      }
    }
  }
  pt::write_xml(xmlOutput, tree);
  response = xmlOutput.str();
  return response;
}

