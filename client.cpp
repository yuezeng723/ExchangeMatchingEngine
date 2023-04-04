#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>
#include <string>


void communicateXML(int sockfd, std::string directory) {
    // send XML file to server
    char buffer[1024] = {0};
    std::vector<std::string> filenames = {"createAccount3.xml", "openSellOrder1.xml"};
    for (const auto& filename : filenames) {
        std::ifstream file("./"+directory+ "/" + filename);
        if (file.is_open()) {
            std::string fileContent;
            std::string line;
            while (std::getline(file, line)) {
                fileContent += line;
            }
            file.close();
            const char* data = fileContent.c_str();
            send(sockfd, data, strlen(data), 0);
            int bytesReceived = recv(sockfd, buffer, 1024, 0);
            if (bytesReceived < 0) {
                std::cerr << "Failed to receive data" << std::endl;
            } else {
                buffer[bytesReceived] = '\0';
                std::string response = buffer;
                std::cout << "Received response: " << response << std::endl;
                // store response in file
                std::ofstream outfile("clientResult.xml");
                if (outfile.is_open()) {
                    outfile << response;
                    outfile.close();
                    std::cout << "Response stored in clientResult.xml" << std::endl;
                } else {
                    std::cerr << "Failed to open file for writing" << std::endl;
                }
            }
            memset(buffer, 0, sizeof(buffer));
        } else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }
}

int main()
{
    const char* hostname = "localhost";
    const char* port = "12345";

    int sockfd;
    struct addrinfo hints, *host_info_list, *p;
    
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
 
    // send a message to the server
    std::string directory = "data";
    communicateXML(sockfd, directory);
    // use sockfd for further communication with the server
    close(sockfd);

    return 0;
}
