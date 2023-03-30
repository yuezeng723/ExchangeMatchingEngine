#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>

#define PORT 12345
#define MAX_CLIENTS 5

void* handleClient(void* arg)
{
    int client_fd = *(int*)arg;
    char buffer[1024] = {0};
    int valread = read( client_fd , buffer, 1024);
    printf("%s\n",buffer );
    send(client_fd , "Hello from server" , strlen("Hello from server") , 0 );
    printf("Hello message sent\n");
    close(client_fd);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<pthread_t> threads;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while(true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
        {
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
    return 0;
}