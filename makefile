net_source = src/extern/*.cpp src/extern/*.h

ext2_source = src/*.h
CCFLAGS = -Ofast -std=c++14

ifeq ($(OS),Windows_NT)
	CCFLAGS += -D WINDOWS -lWs2_32
endif

mkdir:
	mkdir -p bin

server: mkdir
	g++ ${net_source} ${ext2_source} src/server.cpp -o bin/server ${CCFLAGS}

client: mkdir
	g++ ${net_source} src/client.cpp -o bin/client ${CCFLAGS}

clean:
	rm -f bin/server bin/client

all: clean server client

run: server
	./bin/server
