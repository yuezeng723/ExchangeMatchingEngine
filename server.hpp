#include <iostream>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <pqxx/pqxx>
#include <fstream>
using namespace std;
using namespace pqxx;


#define PORT 12345
#define MAX_CLIENTS 5

class Server {
private:
    int server_fd;
    int new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<pthread_t> threads;
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

    int handleClient(void *arg) {
        int client_fd = *(int*)arg;
        char buffer[1024] = {0};
        int valread = read(client_fd, buffer, 1024);
        std::cout << buffer << std::endl;
        send(client_fd , "Hello from server" , strlen("Hello from server") , 0 );
        printf("Hello message sent");
        close(client_fd);
        pthread_exit(NULL);
    }

    void start() {
        while(true) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            pthread_t thread;
            if (pthread_create(&thread, NULL, handleClient, (void*)&new_socket) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            threads.push_back(thread);
        }
        for (auto thread : threads) {
            pthread_join(thread, NULL);
        }
    }
    //Initial database
    void createTables();
    int initialDatabase();
    //Database operations
    void addAccount(int account_id, double balance);
    void updateAccount(int account_id, double balance);
    void addPosition(string symbol, int account_id, int shares);
    void updatePosition(string symbol, int account_id, int shares);
    void addOpenOrder(int transaction_id, int shares, double limit_price, string symbol);
    void addExecuteOrder(int transaction_id, int shares, std::time_t time, double execute_price);
    void addCancelOrder(int transaction_id, int shares, std::time_t time);
    void deleteOpenOrder(int open_id);
    void updateOpenOrder(int open_id, int shares);
};