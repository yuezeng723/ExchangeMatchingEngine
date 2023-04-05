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
#include "sqlHandler.hpp"
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
    sqlHandler * handler;
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
    //Initial database
    void createTables();
    void initialDatabase();
    //Database operations
    void doCreate(int account_id, double balance, string sym, int shares);
    bool orderCheck(int account_id, int amount, double limit);
    void doOrder(int transaction_id, int account_id, string symbol, int amount, double limit);
    result orderMatch(string symbol, int amount, double limit);
    void doQueryOpen(int transaction_id);
    void doQueryExecute(int transaction_id);
    void doQueryCancel(int transaction_id);
    result searchForCancel(int transaction_id);
    void doCancel(int transaction_id, int account_id);

};