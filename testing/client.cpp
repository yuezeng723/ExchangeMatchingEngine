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
#include <mutex>
#include <thread>
namespace pt = boost::property_tree;
//create a mutex lock for xml file
std::mutex xml_lock;
std::mutex terminal_lock;
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
    {
        std::lock_guard<std::mutex> lock(xml_lock);
        char buffer[1024] = {0};
        int received_bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received_bytes > 0) {
            xml_file << "-----------Server response-----------\n" << buffer << std::endl;
        } else if (received_bytes == 0) {
            std::cout << "Server closed connection" << std::endl;
        } else {
            perror("recv");
        }
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
    //create account
    xml_requests.push_back(createAccountPosition(9, "AAPL", 10000, 100));//create account 9 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createAccountPosition(10, "TSL", 20000, 200));//create account 10 with 20000 balance and 200 shares of TSL
    //order matching
    xml_requests.push_back(createOrderTransaction(9, "AAPL", -100, 7)); //account 1 sells 100 shares of AAPL at 7
    xml_requests.push_back(createOrderTransaction(10, "AAPL", 50, 7)); //account 2 buys 50 shares of AAPL at 7
    //cancel order
    xml_requests.push_back(createCancelTransaction(9, 1)); //account 9 cancels order 1
}
void testError_InvalidAccount_orderTransaction (std::vector<std::string>& xml_requests){
    xml_requests.push_back(createOrderTransaction(1000, "AAPL", -100, 7)); //account 1000 does not exist
}

void testError_InvalidAccount_queryTransaction (std::vector<std::string>& xml_requests){
    xml_requests.push_back(createQueryTransaction(1000, 1)); //account 1000 does not exist
}

void testError_InvalidAccount_cancelTransaction (std::vector<std::string>& xml_requests){
    xml_requests.push_back(createCancelTransaction(1000, 1)); //account 1000 does not exist
}

void testError_AccountDoubleCreate(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createAccountPosition(2000, "AAPL", 10000, 100));//create account 1 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createAccountPosition(2001, "AAPL", 10000, 100));//create account 1 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createOrderTransaction(2000, "AAPL", -100, 7)); //account 1 sells 100 shares of AAPL at 7
    xml_requests.push_back(createQueryTransaction(2001, 1));
}

void testError_InvalidTransaction_TransactionNotExist(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createQueryTransaction(1, 200000)); //account 1 does not have transaction 1
}

void testError_InvalidTransaction_CancelTransaction(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createCancelTransaction(1, 200000)); //account 1 does not have transaction 1
}

void testError_InvalidQuery_TransactionNotExist(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createQueryTransaction(1, 200000)); //account 1 does not have transaction 1
}

void testError_InvalidQuery_AccountNotExist(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createQueryTransaction(4000, 1)); //account 1000 does not exist
}

void testError_InvalidQuery_NoPermissionAccount(std::vector<std::string>& xml_requests){
    xml_requests.push_back(createAccountPosition(50, "AAPL", 10000, 100));//create account 1 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createAccountPosition(51, "AAPL", 10000, 100));//create account 1 with 10000 balance and 100 shares of AAPL
    xml_requests.push_back(createQueryTransaction(2, 1)); //account 2 does not have permission to query account 1
}
void handelClient() {
    std::vector<std::string> xml_requests;
    testOrderMatching_FullMatch(xml_requests);
    testOrderMatching_PartialMatch(xml_requests);
    testCancelOrder_FullCancel(xml_requests);
    testCancelOrder_PartialCancel(xml_requests);
    testError_InvalidAccount_orderTransaction(xml_requests);
    testError_InvalidAccount_queryTransaction(xml_requests);
    testError_InvalidAccount_cancelTransaction(xml_requests);
    testError_AccountDoubleCreate(xml_requests);
    testError_InvalidTransaction_CancelTransaction(xml_requests);
    testError_InvalidTransaction_TransactionNotExist(xml_requests);
    testError_InvalidQuery_AccountNotExist(xml_requests);

    std::string server_ip = "127.0.0.1";
    int server_port = 12345;

    for (size_t i = 0; i < xml_requests.size(); ++i) {
        //lock terminal output
        {
            std::lock_guard<std::mutex> lock(terminal_lock);
            std::cout << "Sending " << (i + 1) << ":\n" << std::endl;
        }
        communicateXML(xml_requests[i], server_ip, server_port);
    }
}
int main() {
    std::vector<std::thread> threads;
    int num_threads = 1;//change this number 

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(handelClient));
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
