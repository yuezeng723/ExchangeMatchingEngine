CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -pthread -lboost_system
TARGETS=server

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~

server: main.cpp server.cpp server.hpp sqlHandler.cpp sqlHandler.hpp threadPool.hpp
	$(CC) $(CFLAG) -o $@ $^ $(EXTRAFLAGS)


