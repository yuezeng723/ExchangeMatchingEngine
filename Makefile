CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -pthread -lboost_system
TARGETS=client server

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~
	
client: client.cpp
	g++ -g -o $@ $<

server: main.cpp server.cpp server.hpp 
	$(CC) $(CFLAG) -o $@ $^ $(EXTRAFLAGS)


