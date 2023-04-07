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
    "shares DECIMAL(10,2) NOT NULL,"
    "FOREIGN KEY(account_id) REFERENCES ACCOUNT(account_id)"
    "ON DELETE SET NULL ON UPDATE CASCADE);"; 

  string transactionSQL = 
    "CREATE TABLE TRANSACTION"
    "(id SERIAL PRIMARY KEY,"
    "account_id INT NOT NULL,"
    "FOREIGN KEY(account_id) REFERENCES ACCOUNT(account_id)"
    "ON DELETE SET NULL ON UPDATE CASCADE);";
  string openSQL =
    "CREATE TABLE OPENORDER"
    "(open_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "version integer NOT NULL DEFAULT 1,"
    "shares DECIMAL(10,2) NOT NULL,"
    "limit_price DECIMAL(10,2),"
    "symbol VARCHAR(20) NOT NULL);";
  string executeSQL =
    "CREATE TABLE EXECUTEORDER"
    "(execute_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares DECIMAL(10,2) NOT NULL,"
    "time VARCHAR(30) NOT NULL,"
    "execute_price DECIMAL(10,2));";
  string cancelSQL =
    "CREATE TABLE CANCELORDER"
    "(cancel_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares DECIMAL(10,2) NOT NULL,"
    "time VARCHAR(30) NOT NULL);";

  
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

void Server::handleClient(int client_fd) {
    connection * C1 = new connection("dbname=exchange_matching user=postgres password=ece568");
    sqlHandler * database = new sqlHandler(C1);
    char buffer[4000];
    int bytesReceived;
    while ((bytesReceived = recv(client_fd, buffer, 4000, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        string response = "";
        parseBuffer(database, buffer, bytesReceived, response);
        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
    C1->disconnect();
}

void Server::parseBuffer(sqlHandler * database, char* buffer, int size, string &response) {
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
    handleCreate(database, root, response);
  } else if (rootTag == "transactions") {
    handleTransaction(database, root, response);
  } else {
    pt::ptree tree;
    pt::ptree& treeRoot = tree.add("result", "");
    pt::ptree &error = treeRoot.add("error", "Invalid request");
    error.put("<xmlattr>.id", 0);
    stringstream ss;
    pt::write_xml(ss, tree);
    response = ss.str();
  }
}

string Server::handleCreate(sqlHandler * database, pt::ptree &root, string &response){
  pt::ptree tree;
  pt::ptree& treeRoot = tree.add("result", "");
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("create")) {
    if (v.first == "account") {
      int account_id = v.second.get<int>("<xmlattr>.id");
      double balance = v.second.get<double>("<xmlattr>.balance");
      if (!database->addAccount(account_id, balance)) { //database->checkAccountExist(account_id)
        pt::ptree &error = treeRoot.add("error", "Account already exists");
        error.put("<xmlattr>.id", account_id);
      } else {
        pt::ptree &created = treeRoot.add("created", "");
        created.put("<xmlattr>.id", account_id);
      }
    } else if (v.first == "symbol") {
      string symbol = v.second.get<string>("<xmlattr>.sym");
      int curr_account_id = v.second.get_child("account").get<int>("<xmlattr>.id");
      if (database->checkAccountExist(curr_account_id)) {
        double amount = root.get<double>("create.symbol.account");
        if (database->checkPositionExist(symbol, curr_account_id)) {
          database->updatePosition(symbol, curr_account_id, amount);
        } else {
          database->addPosition(symbol, curr_account_id, amount);
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
  std::stringstream xmlOutput;
  pt::xml_writer_settings<std::string> settings('\t', 1); 
  pt::write_xml(xmlOutput, tree, settings);
  response = xmlOutput.str();
  return response;
}

void Server::responseAccountNotExist(pt::ptree &treeRoot, int account_id) {
  pt::ptree &error = treeRoot.add("error", "Account does not exist");
  error.put("<xmlattr>.id", account_id);
}

void Server::responseOrderTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id) {
  string symbol = v.second.get<string>("<xmlattr>.sym");
  double amount = v.second.get<double>("<xmlattr>.amount");
  double limit_price = v.second.get<double>("<xmlattr>.limit");
  if (amount >= 0 && !database->checkValidBuyOrder(account_id, amount, limit_price)) {
    pt::ptree &error = treeRoot.add("error", "Insufficient balance");
    error.put("<xmlattr>.sym", symbol);
    error.put("<xmlattr>.amount", amount);
    error.put("<xmlattr>.limit", limit_price);
  }
  else if (amount < 0 && !database->checkValidSellOrder(account_id, symbol, amount)) {
    pt::ptree &error = treeRoot.add("error", "Insufficient shares");
    error.put("<xmlattr>.sym", symbol);
    error.put("<xmlattr>.amount", amount);
    error.put("<xmlattr>.limit", limit_price);
  }
  else {
    int transaction_id = database->doOrder(account_id, symbol, amount, limit_price);
    pt::ptree &opened = treeRoot.add("opened", "");
    opened.put("<xmlattr>.sym", symbol);
    opened.put("<xmlattr>.amount", amount);
    opened.put("<xmlattr>.limit", limit_price);
    opened.put("<xmlattr>.id", transaction_id);
  }
}

void Server:: responseQueryTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id) {
  int transaction_id = v.second.get<int>("<xmlattr>.id");
  if (account_id != database->getAccount(transaction_id)) {
    pt::ptree &error = treeRoot.add("error", "Invalid transaction id");
    error.put("<xmlattr>.id", transaction_id);
  } else {
    result openedOrders = database->doQueryOpen(transaction_id);
    result executedOrders = database->doQueryExecute(transaction_id);
    result canceledOrders = database->doQueryCancel(transaction_id);
    pt::ptree &status = treeRoot.add("status", "");
    status.put("<xmlattr>.id", transaction_id);
    if (!openedOrders.empty()) {
      for (const auto &order : openedOrders) {
        pt::ptree &status_open = status.add("open", "");
        status_open.put("<xmlattr>.shares", order["shares"].as<double>());
      }
    }
    if (!canceledOrders.empty()) {
      for (const auto &order : canceledOrders) {
        pt::ptree &status_cancel = status.add("canceled", "");
        status_cancel.put("<xmlattr>.shares", order["shares"].as<double>());
        status_cancel.put("<xmlattr>.time", order["time"].as<string>());
      }
    }
    if (!executedOrders.empty()) {
      for (const auto &order : executedOrders) {
        pt::ptree &status_exec = status.add("executed", "");
        status_exec.put("<xmlattr>.shares", order["shares"].as<double>());
        status_exec.put("<xmlattr>.price", order["execute_price"].as<double>());
        status_exec.put("<xmlattr>.time", order["time"].as<string>());
      }
    }
    if (openedOrders.empty() && executedOrders.empty() && canceledOrders.empty()) {
      pt::ptree &error = treeRoot.add("error", "No such transaction");
      error.put("<xmlattr>.id", transaction_id);
    }
  }
}

void Server::responseCancelTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id){
  int transaction_id = v.second.get<int>("<xmlattr>.id");
  if (account_id != database->getAccount(transaction_id)) {
  pt::ptree &error = treeRoot.add("error", "Invalid transaction id");
  error.put("<xmlattr>.id", transaction_id);
  } else {
    if (!database->checkOpenOrderExist(transaction_id)) {
      pt::ptree &error = treeRoot.add("error", "No open order for canellation");
      error.put("<xmlattr>.id", transaction_id);
    } else {
      database->doCancel(transaction_id, account_id);
      pt::ptree &canceled = treeRoot.add("canceled", "");
      canceled.put("<xmlattr>.id", transaction_id);
      result canceledOrders = database->doQueryCancel(transaction_id);
      for (const auto &order : canceledOrders) {
        pt::ptree &status_cancel = canceled.add("canceled", "");
        status_cancel.put("<xmlattr>.shares", order["shares"].as<double>());
        status_cancel.put("<xmlattr>.time", order["time"].as<string>());
      }
      result executedOrders = database->doQueryExecute(transaction_id);
      for (const auto &order : executedOrders) {
        pt::ptree &status_exec = canceled.add("executed", "");
        status_exec.put("<xmlattr>.shares", order["shares"].as<double>());
        status_exec.put("<xmlattr>.price", order["execute_price"].as<double>());
        status_exec.put("<xmlattr>.time", order["time"].as<string>());
      }
    }
  }
}

string Server::handleTransaction(sqlHandler * database, pt::ptree &root, string &response){
  pt::ptree tree;
  pt::ptree& treeRoot = tree.add("result", "");
  int account_id = root.get<int>("transactions.<xmlattr>.id");
  stringstream xmlOutput;
  BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("transactions")) {
    if (v.first == "order") {
      if (!database->checkAccountExist(account_id)) {
        responseAccountNotExist(treeRoot, account_id);
      } else {
        responseOrderTransaction(database, v, treeRoot, account_id);
      }
    } 
    else if (v.first == "query") {
      if (!database->checkAccountExist(account_id)) {
        responseAccountNotExist(treeRoot, account_id);
      } else {
        responseQueryTransaction(database, v, treeRoot, account_id);
      }
    }
    else if (v.first == "cancel") {
      if (!database->checkAccountExist(account_id)) {
        responseAccountNotExist(treeRoot, account_id);
      } else {
        responseCancelTransaction(database, v, treeRoot, account_id);
      }
    }
  }
  
  pt::xml_writer_settings<std::string> settings('\t', 1); 
  pt::write_xml(xmlOutput, tree, settings);
  response = xmlOutput.str();
  return response;
}
