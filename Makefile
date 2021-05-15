CC=gcc
CCFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror

all: maint stat server client

server : server.o utils_v10.o
	$(CC) $(CCFLAGS) -o server server.o utils_v10.o
	mv server.o Project_Server/
	mv server Project_Server/

client : client.o utils_v10.o
	$(CC) $(CCFLAGS) -o client client.o utils_v10.o
	mv client.o Project_Client/
	mv client Project_Client/

maint : maint.o utils_v10.o
	$(CC) $(CCFLAGS) -o maint maint.o utils_v10.o
	mv maint.o Project_Server/
	mv maint Project_Server/

stat : stat.o utils_v10.o
	$(CC) $(CCFLAGS) -o stat stat.o utils_v10.o
	mv stat.o Project_Server/
	mv stat Project_Server/

server.o: Project_Server/server.c utils_v10.h communications.h Project_Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Project_Server/server.c

client.o: Project_Client/client.c utils_v10.h communications.h
	$(CC) $(CCFLAGS) -c Project_Client/client.c

maint.o: Project_Server/maint.c utils_v10.h Project_Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Project_Server/maint.c

stat.o: Project_Server/stat.c utils_v10.h Project_Server/ipc_conf.h
	$(CC) $(CCFLAGS) -c Project_Server/stat.c

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
