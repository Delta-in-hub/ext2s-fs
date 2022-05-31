net_source = src/extern/*.cpp src/extern/*.h

ext2_source = src/*.hpp
CCFLAGS = -Ofast -std=c++14 -lpthread
TARGET = bin/server bin/client
ifeq ($(OS),Windows_NT)
	CCFLAGS += -D WINDOWS -lWs2_32
	TARGET = bin/server.exe bin/client.exe
endif

mkdir:
	mkdir -p bin

server: mkdir
	g++ ${net_source} ${ext2_source} src/server.cpp -o bin/server ${CCFLAGS}

client: mkdir
	g++ ${net_source} src/client.cpp -o bin/client ${CCFLAGS}

clean:
	rm -f ${TARGET}

all: clean server client

run: server
	./bin/server
