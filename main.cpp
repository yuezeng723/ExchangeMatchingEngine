#include <iostream>
#include <pqxx/pqxx>
#include <fstream>

using namespace std;
using namespace pqxx;

void createTables(connection *C) {
  work W(*C);

  string accountSQL =
    "CREATE TABLE ACCOUNT"
    "(account_id SERIAL PRIMARY KEY,"
    "balance DECIMAL(10,2) NOT NULL,"
    "position_id INT NOT NULL,"
    "FOREIGN KEY(position_id) REFERENCES POSITION(position_id)"
    "ON DELETE SET NULL ON UPDATE CASCADE);"; 

  string positionSQL =
    "CREATE TABLE POSITION"
    "(position_id SERIAL PRIMARY KEY,"
    "symbol VARCHAR(20) NOT NULL,"
    "shares INT NOT NULL);"; 

  string openSQL =
    "CREATE TABLE OPENORDER"
    "(open_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "limit_price DECIMAL(10,2),"
    "symbol VARCHAR(20) NOT NULL;";
  string executeSQL =
    "CREATE TABLE EXECUTEORDER"
    "(execute_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "time TIME,"
    "execute_price DECIMAL(10,2);";
  string cancelSQL =
    "CREATE TABLE CANCELORDER"
    "(cancel_id SERIAL PRIMARY KEY,"
    "transaction_id INT NOT NULL,"
    "shares INT NOT NULL,"
    "time TIME;";

  
  W.exec(positionSQL);
  W.exec(accountSQL);
  W.exec(openSQL);
  W.exec(executeSQL);
  W.exec(cancelSQL);
  W.commit();
}

int main (int argc, char *argv[]) 
{
  connection *C;

  try{
    C = new connection("dbname=exchange_matching user=postgres password=ece568");
    if (C->is_open()) {
      // cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }


    try{
      string dropSql = "DROP TABLE IF EXISTS POSITION, ACCOUNT, OPENORDER, EXECUTEORDER, CACELORDER;";
      /* Create a transactional object. */
      work W(*C);
      /* Execute drop */
      W.exec(dropSql);
      W.commit();
      createTables(C);
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }


  //Close database connection
  C->disconnect();

  return 0;
}
