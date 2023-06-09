#include <iostream>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <vector>
#include <pqxx/pqxx>
#include <fstream>
using namespace std;
using namespace pqxx;

class sqlHandler {
private:
    connection *C;
public:
    sqlHandler(connection * C) : C(C) {
    }
    ~sqlHandler() {
        delete C;
    }

    //Database operations
    
//Database operations
    int addTransaction(int account_id);
    int getAccount(int transaction_id);
    bool addAccount(int account_id, double balance);
    void updateAccount(int account_id, double addon);
    void addPosition(string symbol, int account_id, double shares);
    void updatePosition(string symbol, int account_id, double shares);
    void addOpenOrder(int transaction_id, double shares, double limit_price, string symbol);
    void addExecuteOrder(int transaction_id, double shares, string time, double execute_price, double limit);
    void addCancelOrder(int transaction_id, double shares, string time);
    bool deleteOpenOrder(int open_id, int version);
    bool updateOpenOrder(int open_id, double shares, int version);
    bool checkAccountExist(int account_id);
    bool checkPositionExist(string symbol, int account_id);
    bool checkOpenOrderExist(int transaction_id);
    bool checkValidBuyOrder(int account_id, double amount, double limit);
    bool checkValidSellOrder(int account_id, string symbol, double amount);

    //Logical operations
    int doOrder(int account_id, string symbol, double amount, double limit);
    bool handleMatch(result::const_iterator c, double shares, double amount, int transaction_id, string symbol, int account_id, double currLimit);
    result orderMatch(string symbol, double amount, double limit);
    result doQueryOpen(int transaction_id);
    result doQueryExecute(int transaction_id);
    result doQueryCancel(int transaction_id);
    result searchForCancel(int transaction_id);
    bool doCancel(int transaction_id, int account_id);
};
