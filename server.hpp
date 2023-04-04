#include <iostream>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <pqxx/pqxx>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/foreach.hpp>
namespace pt = boost::property_tree;
using namespace std;
using namespace pqxx;

#define PORT 12345
#define MAX_CLIENTS 5

class Server {
private:
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    connection *C;
public:
    Server() {
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( PORT );
        
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, MAX_CLIENTS) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        initialDatabase();
        createTables();
    }
    ~Server() {
        close(server_fd);
        C->disconnect();
    }

    void handleClient(int new_socket);

    void start() {
        while(true) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            thread clientHandleThread([this, new_socket]() {
                handleClient(new_socket);
            });
            clientHandleThread.detach();
        }
    }
    //handler
    void parseBuffer(char* buffer, int size, string &response);
    string handleCreate(pt::ptree &root, string &result);
    string handleTransaction(pt::ptree &root, string &result);
    //Initial database
    void createTables();
    void initialDatabase();
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
    bool checkAccountExist(int account_id);
    bool checkPositionExist(string symbol, int account_id);
    bool checkOpenOrderExist(int transaction_id);
    //transaction operations
    // void doCreate(int account_id, double balance, string sym, int shares);
    bool checkValidBuyOrder(int account_id, int amount, double limit);
    bool checkValidSellOrder(int account_id, string symbol, int amount);

    void doOrder(int transaction_id, int account_id, string symbol, int amount, double limit);
    result orderMatch(string symbol, int amount, double limit);
    result doQueryOpen(int transaction_id);
    result doQueryExecute(int transaction_id);
    result doQueryCancel(int transaction_id);
    result searchForCancel(int transaction_id);
    void doCancel(int transaction_id, int account_id);

};