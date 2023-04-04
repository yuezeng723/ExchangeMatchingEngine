#include "sqlHandler.hpp"

void sqlHandler::addAccount(int account_id, double balance) {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, balance) VALUES (" << account_id << "," << balance <<");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::updateAccount(int account_id, double addon) {    
    stringstream sql;
    work W(*C);
    sql << "UPDATE ACCOUNT SET balance=balance+" << addon << " WHERE account_id=" << account_id << ";";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addPosition(string symbol, int account_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO POSITION (symbol, account_id, shares) VALUES (" << W.quote(symbol) <<\
    "," << account_id << "," << shares << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::updatePosition(string symbol, int account_id, int shares) { //mutex lock
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
void sqlHandler::addOpenOrder(int transaction_id, int shares, double limit_price, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO openorder (transaction_id, shares, limit_price, symbol) VALUES (" 
    << transaction_id << "," << shares << "," << limit_price << "," << W.quote(symbol) << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addExecuteOrder(int transaction_id, int shares, std::time_t time, double execute_price) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO executeorder (transaction_id, shares, time, execute_price) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << "), " << execute_price << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::addCancelOrder(int transaction_id, int shares, std::time_t time) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO cancelorder (transaction_id, shares, time) VALUES (" 
    << transaction_id << "," << shares << ", " << "to_timestamp(" << W.quote(time) << ")" << ");";
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::deleteOpenOrder(int open_id) {
    stringstream sql;
    work W(*C);
    sql << "DELETE FROM OPENORDER WHERE open_id=" << open_id << ";"; 
    W.exec(sql.str());
    W.commit();
}
void sqlHandler::updateOpenOrder(int open_id, int shares) {
    stringstream sql;
    work W(*C);
    sql << "UPDATE OPENORDER SET shares=shares+" << shares << " WHERE open_id=" << open_id << ";";
    W.exec(sql.str());
    W.commit();
}
