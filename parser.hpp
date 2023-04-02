#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <set>
#include <exception>
#include <iostream>
#include "parser.hpp"
#ifndef _PARSER_
#define _PARSER_

namespace pt = boost::property_tree;
using namespace std;
void parseBuffer(char* buffer, int size);
string handleCreate(pt::ptree &root);
string handleTransaction(pt::ptree &root);