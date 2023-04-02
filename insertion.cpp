#include "insertion.h"
void addAccount(connection *C, int account_id, double balance) {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance <<");";
    W.exec(sql.str());
    W.commit();
}
void updateAccount(connection *C, int account_id, double balance) {    
    stringstream sql;
    work W(*C);
    sql << "UPDATE ACCOUNT SET balance=" << shares << " WHERE account_id=" << account_id << ";";
    W.exec(sql.str());
    W.commit();
}
void addPosition(connection *C, string symbol, int account_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO POSITION (symbol, account_id, shares) VALUES (" << W.quote(symbol) <<\
    "," << account_id << "," << shares << ");";
    W.exec(sql.str());
    W.commit();
}
void updatePosition(connection *C, string symbol, int account_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "UPDATE POSITION SET shares=" << shares << " WHERE account_id=" << account_id <<\
    " AND symbol=" << symbol << ";";
    result R = W.exec(sql.str());
    if(R.affected_rows() == 0) addPosition(C, symbol, account_id, shares);
    W.commit();
}
void addOpenOrder(connection *C, int transaction_id, int shares, double limit_price, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (transaction_id, shares, limit_price, symbol) VALUES (" 
    << transaction_id << "," << shares << "," << limit_price << "," << symbol << ");";
    W.exec(sql.str());
    W.commit();
}
void addExecuteOrder(connection *C, int transaction_id, int shares, std::time_t time, double execute_price) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (transaction_id, shares, time, execute_price) VALUES (" 
    << transaction_id << "," << shares << "," << time << "," << execute_price << ");";
    W.exec(sql.str());
    W.commit();
}
void addCancelOrder(connection *C, int transaction_id, int shares, std::time_t time) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (transaction_id, shares, time) VALUES (" 
    << transaction_id << "," << shares << "," << time << ");";
    W.exec(sql.str());
    W.commit();
}
void deleteOpenOrder(connection *C, int open_id) {
    stringstream sql;
    work W(*C);
    sql << "DELETE FROM OPENORDER WHERE open_id=" << open_id << ";"; 
    W.exec(sql.str());
    W.commit();
}
void updateOpenOrder(connection *C, int open_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "UPDATE OPENORDER SET shares=" << shares << " WHERE order_id=" << order_id << ";"
    W.exec(sql.str());
    W.commit();
}
