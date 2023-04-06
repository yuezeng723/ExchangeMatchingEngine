CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -pthread -lboost_system
TARGETS=testing/client server

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~
	
testing/client: testing/client.cpp
	g++ -g -o $@ $< -pthread

server: main.cpp server.cpp server.hpp sqlHandler.cpp sqlHandler.hpp threadPool.hpp
	$(CC) $(CFLAG) -o $@ $^ $(EXTRAFLAGS)


