CC=gcc
CCFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror

all: maint stat server client

server : server.o utils_v10.o
	$(CC) $(CCFLAGS) -o server server.o utils_v10.o
	mv server.o Server/
	mv server Server/

client : client.o utils_v10.o
	$(CC) $(CCFLAGS) -o client client.o utils_v10.o
	mv client.o Client/
	mv client Client/

maint : maint.o utils_v10.o
	$(CC) $(CCFLAGS) -o maint maint.o utils_v10.o
	mv maint.o Server/
	mv maint Server/

stat : stat.o utils_v10.o
	$(CC) $(CCFLAGS) -o stat stat.o utils_v10.o
	mv stat.o Server/
	mv stat Server/

server.o: Server/server.c utils_v10.h communications.h Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Server/server.c

client.o: Client/client.c utils_v10.h communications.h
	$(CC) $(CCFLAGS) -c Client/client.c

maint.o: Server/maint.c utils_v10.h Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Server/maint.c

stat.o: Server/stat.c utils_v10.h Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Server/stat.c

utils_v10.o: utils_v10.c utils_v10.h
	$(CC) $(CCFLAGS) -c utils_v10.c 


clear :
	clear

clean :
	rm -f *.o
	rm -f server
	rm -f client
	rm -f maint
	rm -f stat
