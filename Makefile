CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq
TARGETS=client server test

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~

test: main.cpp exercise.h exercise.cpp implement.h implement.cpp insertion.h insertion.cpp
	$(CC) $(CFLAGS) -o test main.cpp exercise.cpp implement.cpp insertion.cpp $(EXTRAFLAGS)
	
client: client.cpp
	g++ -g -o $@ $<

server: server.cpp
	g++ -pthread -g -o $@ $<


