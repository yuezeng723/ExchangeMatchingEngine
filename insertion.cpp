
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

// check if the account exists
bool checkAccount(connection *C, int account_id) {
    stringstream sql;
    work W(*C);
    sql << "SELECT * FROM ACCOUNT WHERE account_id = " << account_id << ";";
    result R( W.exec(sql.str()) );
    W.commit();
    if (R.size() == 0) {
        return false;
    }
    return true;
}

// check if the position exists
bool checkPosition(connection *C, string symbol) {
    stringstream sql;
    work W(*C);
    sql << "SELECT * FROM POSITION WHERE symbol = " << W.quote(symbol) << ";";
    result R( W.exec(sql.str()) );
    W.commit();
    if (R.size() == 0) {
        return false;
    }
    return true;
}

