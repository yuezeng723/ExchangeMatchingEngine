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

std::string createAccountPosition(int account_id, const std::string& sym, int balance, double shares) {
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
std::string createOrderTransaction(int account_id, const std::string& sym, int amount, int limit) {
    pt::ptree tree;
    pt::ptree& transactions = tree.add("transactions", "");
    transactions.put("<xmlattr>.id", account_id);
    pt::ptree& order = transactions.add("order", "");
    order.put("<xmlattr>.sym", sym);
    order.put("<xmlattr>.amount", amount);
    order.put("<xmlattr>.limit", limit);

    // pt::ptree& order1 = transactions.add("order", "");
    // order1.put("<xmlattr>.sym", sym);
    // order1.put("<xmlattr>.amount", amount*2);
    // order1.put("<xmlattr>.limit", limit+2);

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

    //open an xml file to store the response
    std::ofstream xml_file;
    xml_file.open("response.xml", std::ios::out | std::ios::app);

    // Buffer to store the server's response
    char buffer[1024] = {0};
    int received_bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (received_bytes > 0) {
        xml_file << "-----------Server response-----------\n" << buffer << std::endl;
    } else if (received_bytes == 0) {
        std::cout << "Server closed connection" << std::endl;
    } else {
        perror("recv");
    }
    close(sockfd);
}
void testCreateAccount(std::vector<std::string>& xml_requests) {
    xml_requests.push_back(createAccountPosition(3, "GOOGLE", 30000, 300));//create account 3 with 30000 balance and 300 shares of AAPL
    xml_requests.push_back(createAccountPosition(4, "META", 40000, 400));//create account 4 with 40000 balance and 400 shares of TSL
}

void testOrderMatching_FullMatch(std::vector<std::string>& xml_requests) {
    xml_requests.push_back(createAccountPosition(1, "AAPL", 10000, 100));//create account 1 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createAccountPosition(2, "TSL", 20000, 200));//create account 2 with 20000 balance and 200 shares of TSL
    xml_requests.push_back(createOrderTransaction(1, "AAPL", -100, 7)); //account 1 sells 100 shares of AAPL at 7
    xml_requests.push_back(createQueryTransaction(1, 1));

    xml_requests.push_back(createOrderTransaction(2, "AAPL", 100, 7)); //account 2 buys 100 shares of AAPL at 7

    xml_requests.push_back(createQueryTransaction(2, 2));
    //the order should be matched
    // xml_requests.push_back(createQueryTransaction(2, 1));
    // xml_requests.push_back(createQueryTransaction(1, 2));

    xml_requests.push_back(createQueryTransaction(1, 1));
    xml_requests.push_back(createQueryTransaction(2, 2));

}

void testOrderMatching_PartialMatch(std::vector<std::string>& xml_requests){
    //create new account with new symbol
    xml_requests.push_back(createAccountPosition(5, "ALPHA", 50000, 500));//create account 5 with 50000 balance and 500 shares of ALPHA
    xml_requests.push_back(createAccountPosition(6, "BETA", 60000, 600));//create account 6 with 60000 balance and 600 shares of BETA
    xml_requests.push_back(createOrderTransaction(5, "ALPHA", -100, 7)); //account 5 sells 100 shares of ALPHA at 7
    xml_requests.push_back(createQueryTransaction(5, 1));

    xml_requests.push_back(createOrderTransaction(6, "ALPHA", 50, 7)); //account 6 buys 50 shares of ALPHA at 7
    xml_requests.push_back(createQueryTransaction(6, 2));
    //query the transaction
    xml_requests.push_back(createQueryTransaction(5, 1));
    xml_requests.push_back(createQueryTransaction(6, 2));
}
void testCancelOrder_FullCancel(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createAccountPosition(7, "TENCENT", 10000, 100));//create account 7 with 10000 balance and 100 shares of TENCENT
    xml_requests.push_back(createAccountPosition(8, "FACEBOOK", 20000, 200));//create account 8 with 20000 balance and 200 shares of FACEBOOK
    //cant match
    xml_requests.push_back(createOrderTransaction(7, "TENCENT", -100, 7)); //account 7 sells 100 shares of TENCENT at 7
    xml_requests.push_back(createOrderTransaction(8, "TENCENT", 100, 2)); //account 8 buys 100 shares of TENCENT at 2
    //cancel order
    xml_requests.push_back(createCancelTransaction(7, 1)); //account 7 cancels order 1
    //query the transaction
    xml_requests.push_back(createQueryTransaction(7, 1));
}

void testCancelOrder_PartialCancel(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createOrderTransaction(1, "AAPL", -100, 7)); //account 1 sells 100 shares of AAPL at 7
    xml_requests.push_back(createOrderTransaction(2, "AAPL", 50, 7)); //account 2 buys 50 shares of AAPL at 7
    xml_requests.push_back(createCancelTransaction(1, 1)); //account 1 cancels order 1
}
int main() {
    std::vector<std::string> xml_requests;
    // xml_requests.push_back(createAccountPosition(1, "AAPL", 5000, 100));
    // //add
    // xml_requests.push_back(createAccountPosition(2, "AAPL", 1000, 100));

    // xml_requests.push_back(createOrderTransaction(1, "AAPL", 100, 7));

    // xml_requests.push_back(createOrderTransaction(1, "TSL", 200, 5));
    // //add
    // xml_requests.push_back(createOrderTransaction(2, "AAPL", -100, 4));
    // xml_requests.push_back(createQueryTransaction(1, 2));
    // xml_requests.push_back(createQueryTransaction(2, 4));
    // xml_requests.push_back(createQueryTransaction(2, 5));
//origin
    // xml_requests.push_back(createQueryTransaction(1, 1));

    // xml_requests.push_back(createQueryTransaction(1, 2));

    // xml_requests.push_back(createCancelTransaction(1, 1));

    // xml_requests.push_back(createCancelTransaction(1, 2));


    // testCreateAccount(xml_requests);
    testOrderMatching_FullMatch(xml_requests);
    // testOrderMatching_PartialMatch(xml_requests); 

    std::string server_ip = "127.0.0.1";
    int server_port = 12345;


    for (size_t i = 0; i < xml_requests.size(); ++i) {
        std::cout << "Sending XML Request " << (i + 1) << ":\n" << xml_requests[i] << std::endl;
        communicateXML(xml_requests[i], server_ip, server_port);
    }

    return 0;
}
