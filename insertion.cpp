
void addAccount(connection *C, int account_id, double balance, int position_id) {    
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (account_id, balance, position_id) VALUES (" << account_id << "," << balance << ","
    << position_id <<");";
    W.exec(sql.str());
    W.commit();
}
void addPosition(connection *C, string symbol, int shares) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (symbol, shares) VALUES (" << W.quote(symbol) << "," << shares << ");";
    W.exec(sql.str());
    W.commit();
}
void addOrder(connection *C, int transaction_id, int account_id, string status, int shares, std::time_t time, \
double limit_price, double execute_price, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "INSERT INTO ACCOUNT (transaction_id, account_id, status, shares, time, limit_price, execute_price, symbol) VALUES (" 
    << transaction_id << "," << account_id << "," << W.quote(status) << "," << shares << "," << time << "," << limit_price << "," << execute_price << "," << 
    << "," << symbol << ");";
    W.exec(sql.str());
    W.commit();
}