#include "parser.hpp"

//read the xml file from the buffer, and check the its root tag is create or transaction
void parseBuffer(char* buffer, int size) {
    pt::ptree root;
    stringstream ss;
    ss.write(buffer, size);
    pt::read_xml(ss, root);
    string rootTag = root.begin()->first;
    if (rootTag == "create") {
        handleCreate(root);
    } else if (rootTag == "transaction") {
        handleTransaction(root);
    } else {
        throw runtime_error("Invalid XML file");
    }
}

string handleCreate(pt::ptree &root){
    string result = "";
    result += "<result>";
    BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("create")) {
        if (v.first == "account") {
            int account_id = v.second.get<int>("<xmlattr>.id");
            double balance = v.second.get<double>("<xmlattr>.balance");
            if (checkAccount(account_id)) {
                result += "<error id=\"" + to_string(account_id) + "\">Account already exists</error>";
            } else {
                addAccount(account_id, balance);
            }
        } else if (v.first == "position") {
            string symbol = v.second.get<string>("<xmlattr>.symbol");
            int amount = v.second.get<int>("<xmlattr>.amount");
            if (checkPosition(symbol)) {
                updatePosition(symbol, amount);
            } else {
                addPosition(symbol, amount);
            }
        }
    }
    result += "</result>";
    return result;
}

string handleTransaction(pt::ptree &root){
    string result = "";
    result += "<result>";
    int account_id = root.get<int>("<xmlattr>.id");
    if (!checkAccount(account_id)) {
        result += "<error id=\"" + to_string(account_id) + "\">Account does not exist</error>";
        return result;
    }
    BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("transaction")) {
        if (v.first == "order") {
            string symbol = v.second.get<string>("<xmlattr>.sym");
            int amount = v.second.get<int>("<xmlattr>.amount");
            double limit_price = v.second.get<double>("<xmlattr>.limit");
            int order_id = addOrder(account_id, amount, limit_price, symbol);
            placeOrder(order_id, amount, limit_price, symbol);
            result += "<opened sym=\"" + symbol + "\" amount=\"" + to_string(amount) + "\" limit=\"" + to_string(limit_price) + "\"/>";
        } else if (v.first == "query") {
            vector<tuple<int, int, double, double, string, std::time_t, int>> transactions = queryTransactions(account_id);
            for (auto transaction : transactions) {
                int order_id = get<0>(transaction);
                int amount = get<1>(transaction);
                double limit_price = get<2>(transaction);
                double execute_price = get<3>(transaction);
                string status = get<4>(transaction);
                std::time_t time = get<5>(transaction);
                int position_id = get<6>(transaction);
                result += "<status id=\"" + to_string(order_id) + "\">";
                if (status == "open") {
                    result += "<open shares=\"" + to_string(amount) + "\"/>";
                } else if (status == "canceled") {
                    result += "<canceled shares=\"" + to_string(amount) + "\" time=\"" + to_string(time) + "\"/>";
                } else if (status == "executed") {
                    result += "<executed shares=\"" + to_string(amount) + "\" price=\"" + to_string(execute_price) + "\" time=\"" + to_string(time) + "\"/>";
                }
                result += "</status>";
            }
        } else if (v.first == "cancel") {
            int order_id = v.second.get<int>("<xmlattr>.id");
            cancelOrder(order_id);
            result += "<canceled id=\"" + to_string(order_id) + "\"/>";
            vector<tuple<int, int, double, double, string, std::time_t, int>> transactions = queryOpenTransactions(order_id);
            if (transactions.empty()) {
                result += "<error id=\"" + to_string(order_id) + "\">Order does not exist</error>";
            } else {
                for (auto transaction : transactions) {
                    int amount = get<1>(transaction);
                    std::time_t time = get<5>(transaction);
                    result += "<cancelled shares=\"" + to_string(amount) + "\" time=\"" + to_string(time) + "\"/>";
                }
            }
            transactions = queryExecutedTransactions(order_id);
            if (!transactions.empty()) {
                for (auto transaction : transactions) {
                    int amount = get<1>(transaction);
                    double execute_price = get<3>(transaction);
                    std::time_t time = get<5>(transaction);
                    result += "<executed shares=\"" + to_string(amount) + "\" price=\"" + to_string(execute_price) + "\" time=\"" + to_string(time) + "\"/>";
                }
            }
        }
    }
    result += "</result>";
    return result;
}

