#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

std::string createAccountPosition(int account_id, const std::string& sym, int balance, int shares) {
    pt::ptree tree;
    pt::ptree& create = tree.add("create", "");
    pt::ptree& account = create.add("account", "");
    account.put("<xmlattr>.id", account_id);
    account.put("<xmlattr>.balance", balance);
    if (!sym.empty()) {
        pt::ptree& symbol = create.add("symbol", "");
        symbol.put("<xmlattr>.sym", sym);
        pt::ptree& account_elem = symbol.add("account", "");
        account_elem.put("<xmlattr>.id", account_id);
        account_elem.put_value(shares);
    }
    std::stringstream xml_stream;
    pt::write_xml(xml_stream, tree, pt::xml_writer_make_settings<std::string>('\t', 1));
    std::string xml_request = xml_stream.str();
    int message_size = xml_request.size();
    std::stringstream message_stream;
    message_stream << message_size << std::endl << xml_request;
    return message_stream.str();
}
std::string createOpenTransaction(int account_id, const std::string& sym, int amount, int limit) {
    pt::ptree tree;
    pt::ptree& transactions = tree.add("transactions", "");
    transactions.put("<xmlattr>.id", account_id);
    pt::ptree& order = transactions.add("order", "");
    order.put("<xmlattr>.sym", sym);
    order.put("<xmlattr>.amount", amount);
    order.put("<xmlattr>.limit", limit);
    std::stringstream xml_stream;
    pt::write_xml(xml_stream, tree, pt::xml_writer_make_settings<std::string>('\t', 1));
    std::string xml_request = xml_stream.str();
    int message_size = xml_request.size();
    std::stringstream message_stream;
    message_stream << message_size << std::endl << xml_request;
    return message_stream.str();
}

std::string createQueryTransaction(int account_id, int trans_id) {
    pt::ptree tree;
    pt::ptree& transactions = tree.add("transactions", "");
    transactions.put("<xmlattr>.id", account_id);
    pt::ptree& query = transactions.add("query", "");
    query.put("<xmlattr>.id", trans_id);
    std::stringstream xml_stream;
    pt::write_xml(xml_stream, tree, pt::xml_writer_make_settings<std::string>('\t', 1));
    std::string xml_request = xml_stream.str();
    int message_size = xml_request.size();
    std::stringstream message_stream;
    message_stream << message_size << std::endl << xml_request;
    return message_stream.str();
}

std::string createCancelTransaction(int account_id, int trans_id) {
    pt::ptree tree;
    pt::ptree& transactions = tree.add("transactions", "");
    transactions.put("<xmlattr>.id", account_id);
    pt::ptree& cancel = transactions.add("cancel", "");
    cancel.put("<xmlattr>.id", trans_id);
    std::stringstream xml_stream;
    pt::write_xml(xml_stream, tree, pt::xml_writer_make_settings<std::string>('\t', 1));
    std::string xml_request = xml_stream.str();
    int message_size = xml_request.size();
    std::stringstream message_stream;
    message_stream << message_size << std::endl << xml_request;
    return message_stream.str();
}

void communicateXML(const std::string& xml_request, const std::string& server_ip, int server_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return;
    }
    send(sockfd, xml_request.c_str(), xml_request.size(), 0);

    // Buffer to store the server's response
    char buffer[1024] = {0};
    int received_bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (received_bytes > 0) {
        std::cout << "Server response:\n" << buffer << std::endl;
    } else if (received_bytes == 0) {
        std::cout << "Server closed connection" << std::endl;
    } else {
        perror("recv");
    }
    close(sockfd);
}

int main() {
    std::vector<std::string> xml_requests;
    xml_requests.push_back(createAccountPosition(1, "AAPL", 1000, 100));

    xml_requests.push_back(createOpenTransaction(1, "AAPL", 100, 7));

    xml_requests.push_back(createOpenTransaction(1, "TSL", 200, 5));

    xml_requests.push_back(createQueryTransaction(1, 1));

    xml_requests.push_back(createQueryTransaction(1, 2));

    xml_requests.push_back(createCancelTransaction(1, 1));

    xml_requests.push_back(createCancelTransaction(1, 2));



    std::string server_ip = "127.0.0.1";
    int server_port = 12345;

    for (size_t i = 0; i < xml_requests.size(); ++i) {
        std::cout << "Sending XML Request " << (i + 1) << ":\n" << xml_requests[i] << std::endl;
        communicateXML(xml_requests[i], server_ip, server_port);
    }

    return 0;
}
