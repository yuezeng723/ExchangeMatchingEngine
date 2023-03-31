CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq
TARGETS=client server test

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~

test: main.cpp
	$(CC) $(CFLAGS) -o test main.cpp $(EXTRAFLAGS)
	
client: client.cpp
	g++ -g -o $@ $<

server: server.cpp
	g++ -pthread -g -o $@ $<


