#include "server.hpp"

int main(int argc, char const *argv[]) {
  try {
    Server server;
    server.start();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}