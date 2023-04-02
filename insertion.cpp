#include "insertion.h"
void addAccount(connection *C, double balance) {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance << ");";
    W.exec(sql.str());
    W.commit();
}
void addPosition(connection *C, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (symbol) VALUES (" << W.quote(symbol) << ");";
    W.exec(sql.str());
    W.commit();
}
void addOrder(connection *C, int account_id, int amount, double limit_price, double execute_price, 
string status, std::time_t time, int position_id) {
    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, amount, limit_price, execute_price, status, time, position_id) VALUES (" 
    << account_id << "," << amount << "," << limit_price << "," << execute_price << "," << W.quote(status)
    << "," << time << "," << position_id << ");";
    W.exec(sql.str());
    W.commit();
}


