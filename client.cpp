#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>



int main()
{
    const char* hostname = "localhost";
    const char* port = "12345";

    int sockfd;
    struct addrinfo hints, *host_info_list, *p;
    char buffer[1024] = {0};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever is available
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    if (getaddrinfo(hostname, port, &hints, &host_info_list) != 0) {
        std::cerr << "getaddrinfo failed" << std::endl;
        return -1;
    }

    // loop through all the results and connect to the first one we can
    for (p = host_info_list; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        std::cerr << "Failed to connect" << std::endl;
        return -1;
    }

    freeaddrinfo(host_info_list);
 
    // send XML file to server
    std::vector<std::string> filenames = {"createAccount1.xml", "createAccount2.xml"};
    std::string file_contents;
    for (const auto& filename : filenames) {
        std::ifstream file("./data/" + filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                file_contents += line;
            }
            file.close();
            const char* data = file_contents.c_str();
            send(sockfd, data, strlen(data), 0);

            int valread = read(sockfd, buffer, 1024);
            std::cout << buffer << std::endl;
        } else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    // use sockfd for further communication with the server
    close(sockfd);

    return 0;
}