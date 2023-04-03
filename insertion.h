#include <ctime>
#include <string>
#include <iostream>
#include <pqxx/pqxx>
using namespac
using namespace pqxx;
#ifndef _INSERTION_
#define _INSERTION_

void addAccount(connection *C, int account_id, double balance);
void updateAccount(connection *C, int account_id, double balance);
void addPosition(connection *C, string symbol, int account_id, int shares);
void updatePosition(connection *C, string symbol, int account_id, int shares);
void addOpenOrder(connection *C, int transaction_id, int shares, double limit_price, string symbol);
void addExecuteOrder(connection *C, int transaction_id, int shares, std::time_t time, double execute_price);
void addCancelOrder(connection *C, int transaction_id, int shares, std::time_t time);
void deleteOpenOrder(connection *C, int open_id);
void updateOpenOrder(connection *C, int open_id, int shares);
