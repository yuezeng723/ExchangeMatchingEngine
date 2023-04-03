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
        addPosition(C, symbol, account_id, shares);
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