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
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
  }
}

void Server::addAccount(int account_id, double balance) {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance <<");";
    W.exec(sql.str());
    W.commit();
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
void Server::updatePosition(string symbol, int account_id, int shares) {
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
    sql << "UPDATE OPENORDER SET shares=" << shares << " WHERE open_id=" << open_id << ";";
    W.exec(sql.str());
    W.commit();
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

