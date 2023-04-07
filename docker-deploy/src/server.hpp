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

#include "sqlHandler.hpp"
#include "threadPool.hpp"
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
    connection * C;
    ThreadPool * pool;
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
        pool = new ThreadPool(10);
    }
    ~Server() {
        close(server_fd);
        C->disconnect();
    }

    void start() {
        while(true) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            pool->enqueue([this](int new_socket) { this->handleClient(new_socket); }, new_socket);
        }
    }
    //handler
    void handleClient(int new_socket);
    void parseBuffer(sqlHandler * database, char* buffer, int size, string &response);
    string handleCreate(sqlHandler * database, pt::ptree &root, string &result);
    string handleTransaction(sqlHandler * database, pt::ptree &root, string &result);
    string setFractionNum(double val);
    //write xml response
    void responseAccountNotExist(pt::ptree &treeRoot, int account_id);
    void responseOrderTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id);
    void responseQueryTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id);
    void responseCancelTransaction(sqlHandler * database, pt::ptree::value_type &v, pt::ptree &treeRoot, int account_id);
    //Initial database
    void createTables();
    void initialDatabase();
};
