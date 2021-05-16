#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../utils_v10.h"
#include "../communications.h"

#define BUFFERSIZE 300

volatile sig_atomic_t end = 0;
int port;
char* addr;



// PRE: ServerIP : a valid IP address
//      ServerPort : a valid port number
// POST: on success connects a client socket to ServerIP:port
//       return socket file descriptor
//       on failure, displays error cause and quits the program
int initSocketClient(char ServerIP[16], int Serverport) {
  int sockfd = ssocket();
  sconnect(ServerIP, Serverport, sockfd);
  return sockfd;
}



// PRE: sockfd : a socket file descriptor
// POST: read and print the text
void readServerMessage(int sockfd) {
	char buffer[BUFFERSIZE];
	while(sread(sockfd,&buffer,sizeof(buffer)) != 0){
		printf("%s\n",buffer);
	}
}



// PRE: sockfd : a socket file descriptor
//      filePath : the path of the file to upload
// POST: send the file's text through the socket
void uploadFile(int sockfd, char* filePath){
  int fd = sopen(filePath,O_RDONLY,0100);

  char buffer[BUFFERSIZE];
  while(sread(fd,&buffer,sizeof(buffer)) != 0){
    swrite(sockfd,buffer,sizeof(buffer));
  }
  int s = shutdown(sockfd,SHUT_WR); 
  checkNeg(s, "ERROR SHUTDOWN");

  sclose(fd);
}



// PRE: filePath : a valid IP address
// POST: add a C file on the server
void addFile(char* filePath) {
	int sockfd = initSocketClient(addr, port);

	//find the last occur of '/' in the filePath to get the fileName
	char* fileName = strrchr(filePath, '/')+1;
	

	CommunicationClientServer msg;
	msg.nbCharFilename = strlen(fileName);
	strcpy(msg.filename,fileName);
	msg.num = -1; 			//to distinct with the replace command
	swrite(sockfd,&msg,sizeof(msg));

	//upload and send the file
	uploadFile(sockfd, filePath);

	CommunicationServerClient serverMsg;
	sread(sockfd,&serverMsg,sizeof(serverMsg));
	
	printf("\n-----------------------------------------------\nRéponse du serveur:\n");
	printf("   Numéro du programme: %d\n", serverMsg.num);
	printf("   Compilation programme: %d\n", serverMsg.state);
	if(serverMsg.state != 0){
		printf("   Message d'erreur: \n\n");
		readServerMessage(sockfd);	
	}
	printf("\n----------------------------------------------- \n\n");
	
}


// PRE: num: the num of the programm to replace
//      pathFile : the path of the file to upload
// POST: replace the programm num by a C file on the server
void replaceFile(int num, char* filePath) {
	int sockfd = initSocketClient(addr, port);

	//find the last occur of '/' in the filePath to get the fileName
	char* fileName = strrchr(filePath, '/')+1;
			
	CommunicationClientServer msg;
	msg.num = num;
	msg.nbCharFilename = strlen(fileName);
	strcpy(msg.filename,fileName);
	swrite(sockfd,&msg,sizeof(msg));

	//upload and send the file
	uploadFile(sockfd, filePath);

	CommunicationServerClient serverMsg;
	sread(sockfd,&serverMsg,sizeof(serverMsg));
	printf("\n-----------------------------------------------\nRéponse du serveur:\n");
	printf("   Numéro du programme: %d\n", serverMsg.num);
	printf("   Compilation programme: %d\n", serverMsg.state);
	if(serverMsg.state != 0){
		printf("   Message d'erreur: \n\n");
		readServerMessage(sockfd);	
	}
	printf("\n----------------------------------------------- \n\n");
}


// PRE: num: the num of the programm to execute
// POST: execute the num programm on the server
void execProg(int num) {
	int sockfd = initSocketClient(addr, port);

	CommunicationClientServer msg;
	msg.num = num;
	msg.nbCharFilename = -1;		//to distinct with the add and replace commands
	swrite(sockfd,&msg,sizeof(msg));

	CommunicationServerClient serverMsg;
	sread(sockfd,&serverMsg,sizeof(serverMsg));
	
	printf("\n-----------------------------------------------\nRéponse du serveur:\n");
	printf("   Numéro du programme: %d\n", serverMsg.num);
	printf("   État du programme: %d\n", serverMsg.state);
	printf("   Temps d'exécution: %d\n", serverMsg.executionTime);
	printf("   Code de retour: %d\n", serverMsg.returnCode);
	printf("   Sortie standard: \n\n");		
	readServerMessage(sockfd);	
	
	printf("\n-----------------------------------------------\n");
}


//***************************************************************************
// TIMER
//***************************************************************************

void timer(void *arg1, void* arg2) {
	int *pipefd = arg1;
	int delay = *(int*)arg2;

	int heartBit = -1;

	// close read
	sclose(pipefd[0]);

	// while no signal
	while (end == 0) {
		sleep(delay);
		swrite(pipefd[1], &heartBit, sizeof(int));
	}
	
	// close write
	sclose(pipefd[1]);
}


//***************************************************************************
// RECUREXEC
//***************************************************************************
 
void recurExec(void *arg1) {
	int *pipefd = arg1;

	int num;
	int progs[100];
	int nbProgs = 0;
	
	// close write
	int ret = close(pipefd[1]);
	checkNeg(ret, "close error");

	// read pipe
	while(sread(pipefd[0], &num, sizeof(int)) > 0){
		if (num < 0) {
			// execute all progs (every heartBit from timer)
			for (int i = 0; i < nbProgs; ++i) {
				execProg(progs[i]);
			}			
		} else {
			// command * (add a progNum in RecurExec)
			progs[nbProgs] = num;
			nbProgs++;
		}
	}

	//close read
	sclose(pipefd[0]);
}



// signal to close timer's pipe
void sigusr1_handler() {
  end = 1;
}


//***************************************************************************
// TERMINAL
//***************************************************************************

int main(int argc, char **argv){
	addr = argv[1];
	port = atoi(argv[2]);
	int delay = atoi(argv[3]);
	ssigaction(SIGUSR1, sigusr1_handler);

	/* create pipe */
	int pipefd[2];
	spipe(pipefd);

	/* create child 1 */
	pid_t pidTimer = fork_and_run2(timer, pipefd, &delay);

	/* create child 2 */
	fork_and_run1(recurExec, pipefd);

	//close read
	sclose(pipefd[0]);

	/* read on stdin */
	char bufRd[BUFFERSIZE];
	int nbCharRd = sread(0, bufRd, BUFFERSIZE);
	char command = bufRd[0];
	bufRd[nbCharRd-1] = '\0';
	char* param = bufRd+2;

	/* prompt */
	while(command != 'q') {
		/* add a C file to the server */
		if (command == '+') {
			addFile(param);
		}
		/* replace a C file to the server */
		else if (command == '.') {
			//split 2 parameters
			char* spaceAddress = strchr(param, ' ');
			char* filePath = spaceAddress+1;
			*spaceAddress = '\0';
			int num = atoi(param);
			replaceFile(num, filePath);
		}
		/* add a progNum in RecurExec */
		else if (command == '*') {
			int num = atoi(param);
			//send the progNum to the RecurExec through the pipe
			swrite(pipefd[1], &num, sizeof(int));
		}
		/* exec once a progNum */
		else if (command == '@') {
			int num = atoi(param);
			execProg(num);
		}

		/* read on stdin */
		nbCharRd = sread(0, bufRd, BUFFERSIZE);
		command = bufRd[0];
		bufRd[nbCharRd-1] = '\0';
		param = bufRd+2;
	}

	// send signal to close timer's pipe
	skill(pidTimer, SIGUSR1);

	// close write
	sclose(pipefd[1]);

	return 0;
}









