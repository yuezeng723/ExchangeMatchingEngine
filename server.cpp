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
        handler->updateOpenOrder(c[1].as<int>(), amount);
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
  stringstream bufferStream;
  bufferStream.write(buffer, size);
  string firstLine;
  bufferStream >> firstLine;
  string content;
  getline(bufferStream, content, '\0');
  int content_size;
  try {
    content_size = stoi(firstLine);
  } catch (const std::invalid_argument& e) {
    throw runtime_error("Invalid content size");
  }
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
  } else if (rootTag == "transaction") {
    handleTransaction(root, response);
  } else {
    throw runtime_error("Invalid XML file in parseBuffer");
  }
}

string Server::handleCreate(pt::ptree &root, string &result){
  result += "<result>";
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("create")) {
    if (v.first == "account") {
      int account_id = v.second.get<int>("<xmlattr>.id");
      double balance = v.second.get<double>("<xmlattr>.balance");
      if (checkAccountExist(account_id)) {
        result += "<error id=\"" + to_string(account_id) + "\">Account already exists</error>";
      } else {
        addAccount(account_id, balance);
        result += "<created id=\"" + to_string(account_id) + "\"/>";
      }
    } else if (v.first == "symbol") {
      string symbol = v.second.get<string>("<xmlattr>.sym");
      //find the child node of position which is "account", get the value inside the node and get the attribute "id"
      int curr_account_id = v.second.get_child("account").get<int>("<xmlattr>.id");
      if (checkAccountExist(curr_account_id)) {
        int amount = root.get<int>("create.symbol.account");
        if (checkPositionExist(symbol, curr_account_id)) {
          updatePosition(symbol, curr_account_id, amount);
        } else {
          addPosition(symbol, curr_account_id, amount);
        }
        result += "<created sym=\"" + symbol + "\" id=\"" + to_string(curr_account_id) + "\"/>";
      } else {
        result += "<error sym=\"" + symbol + "id=\"" + to_string(curr_account_id) + "\">Account does not exist</error>";
      }
    }
  }
  result += "</result>";
  return result;
}


string Server::handleTransaction(pt::ptree &root, string &result){
  result += "<result>";
  int account_id = root.get<int>("<xmlattr>.id");
  if (!checkAccount(C, account_id)) {
    result += "<error id=\"" + to_string(account_id) + "\">Account does not exist</error>";
    return result;
  }
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("transaction")) {
    if (v.first == "order") {
      string symbol = v.second.get<string>("<xmlattr>.sym");
      int amount = v.second.get<int>("<xmlattr>.amount");
      double limit_price = v.second.get<double>("<xmlattr>.limit");
      int order_id = addOrder(C, account_id, amount, limit_price, symbol);
      placeOrder(C, order_id, amount, limit_price, symbol);
      result += "<opened sym=\"" + symbol + "\" amount=\"" + to_string(amount) + "\" limit=\"" + to_string(limit_price) + "\"/>";
      //start matching

    } else if (v.first == "query") {
      vector<tuple<int, int, double, double, string, std::time_t, int>> transactions = queryTransactions(C, account_id);
      for (auto transaction : transactions) {
        int order_id = get<0>(transaction);
        int amount = get<1>(transaction);
        double limit_price = get<2>(transaction);
        double execute_price = get<3>(transaction);
        string status = get<4>(transaction);
        std::time_t time = get<5>(transaction);
        int position_id = get<6>(transaction);
        result += "<status id=\"" + to_string(order_id) + "\">";
        if (status == "open") {
          result += "<open shares=\"" + to_string(amount) + "\"/>";
        } else if (status == "canceled") {
          result += "<canceled shares=\"" + to_string(amount) + "\" time=\"" + to_string(time) + "\"/>";
        } else if (status == "executed") {
          result += "<executed shares=\"" + to_string(amount) + "\" price=\"" + to_string(execute_price) + "\" time=\"" + to_string(time) + "\"/>";
        }
        result += "</status>";
      }
    } else if (v.first == "cancel") {
      int order_id = v.second.get<int>("<xmlattr>.id");
      cancelOrder(C, order_id);// DATABASE: cancel the opened part of the order
      result += "<canceled id=\"" + to_string(order_id) + "\"/>";
      vector<tuple<int, int, double, double, string, std::time_t, int>> transactions = queryOpenTransactions(order_id);
      if (transactions.empty()) {
        result += "<error id=\"" + to_string(order_id) + "\">Order does not exist</error>";
      } else {
        for (auto transaction : transactions) {
          int amount = get<1>(transaction);
          std::time_t time = get<5>(transaction);
          result += "<cancelled shares=\"" + to_string(amount) + "\" time=\"" + to_string(time) + "\"/>";
        }
      }
      transactions = queryExecutedTransactions(C, order_id);
      if (!transactions.empty()) {
        for (auto transaction : transactions) {
          int amount = get<1>(transaction);
          double execute_price = get<3>(transaction);
          std::time_t time = get<5>(transaction);
          result += "<executed shares=\"" + to_string(amount) + "\" price=\"" + to_string(execute_price) + "\" time=\"" + to_string(time) + "\"/>";
        }
      }
    }
  }
  result += "</result>";
  return result;
}

