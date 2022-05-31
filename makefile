net_source = src/extern/*.cpp src/extern/*.h

ext2_source = src/*.h

server:
	g++ ${net_source} ${ext2_source} src/server.cpp -o bin/server

client:
	g++ ${net_source} src/client.cpp -o bin/client

clean:
	rm -f bin/server bin/client

all: clean server client

run: server
	./bin/server
