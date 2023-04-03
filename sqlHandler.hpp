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
    }

    //Database operations
    void addAccount(int account_id, double balance);
    void updateAccount(int account_id, double addon);
    void addPosition(string symbol, int account_id, int shares);
    void updatePosition(string symbol, int account_id, int shares);
    void addOpenOrder(int transaction_id, int shares, double limit_price, string symbol);
    void addExecuteOrder(int transaction_id, int shares, std::time_t time, double execute_price);
    void addCancelOrder(int transaction_id, int shares, std::time_t time);
    void deleteOpenOrder(int open_id);
    void updateOpenOrder(int open_id, int shares);
};