#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils_v10.h"
#include "../communications.h"

#define BUFFERSIZE 300

volatile sig_atomic_t end = 0;
char* adr;
int port;
//TODO: Verif declarat° variable globale + passage param inutile


int initSocket() {
	//TODO
}



void addFile(char* filePath) {
	int sockfd = initSocket();


	char* fileName = strchr(filePath, int c'/')+1;
			
	CommunicationClientServer msg;
	msg.nbCharFilename = strlen(fileName);
	msg.filename[0] = fileName;
	swrite(sockfd,&msg,sizeof(msg));

	uploadFile(sockfd, filePath)

	/*TODO: print ServerResponse*/
}


void replaceFile(int num, char* filePath) {
	int sockfd = initSocket();


	char* fileName = strchr(filePath, int c'/')+1;
			
	CommunicationClientServer msg;
	msg.num = num;
	msg.nbCharFilename = strlen(fileName);
	msg.filename[0] = fileName;
	swrite(sockfd,&msg,sizeof(msg));

	uploadFile(sockfd, filePath)

	/*TODO: print ServerResponse*/
}


void execProg(int num) {
	int sockfd = initSocket();


	CommunicationClientServer msg;
	msg.num = num;
	swrite(sockfd,&msg,sizeof(msg));

	/*TODO: print ServerResponse*/
}



void uploadFile(int sockfd, char* pathFile){
  int fd = sopen(pathFile,O_RDONLY,0100);

  char buffer[1000];
  while(sread(fd,&buffer,sizeof(buffer)) != 0){
    swrite(sockfd,buffer,sizeof(buffer));
  }
  int s = shutdown(sockfd,SHUT_RD); //A VOIR POUR PEUT ETRE DISALLOWED SHUT-RDWR
  checkNeg(s, "ERROR SHUTDOWN");

  sclose(fd);
}





//***************************************************************************
// TERMIINAL
//***************************************************************************

int main(int argc, char **argv){

	adr = argv[0];
	port = atoi(argv[1]);
	int delay = atoi(argv[2]);

	/* create pipe */
	int pipefd[2];
	int ret = pipe(pipefd);
	checkNeg(ret, "pipe error");

	/* create child 1 */
	fork_and_run2(timer, pipefd, &delay);

	/* create child 2 */
	fork_and_run3(timer, pipefd, &adr, &port);

	//close read
	ret = close(pipefd[0]);
	checkNeg(ret, "close error");

	/* read on stdin */
	char bufRd[BUFFERSIZE];
	int nbCharRd = sread(0, bufRd, BUFFERSIZE);
	char command = bufRd[0];
	bufRd[nbCharRd-1] = '\0';
	char* param = bufRd+2;

	/* prompt */
	while(command != 'q') {
		//add a C file to the server
		if (command == '+') {
			addFile(param);
		}
		//replace a C file to the server
		if else (command == '.') {
			//split 2 param
			char* spaceAddress = strtok(param, ' ');
			char* filePath = spaceAddress+1;
			*spaceAddress = '\0';
			int num = atoi(param);

			replaceFile(num, filePath);
		}
		//add a progNum in RecurExec
		if else (command == '*') {
			int num = atoi(param);
			swrite(pipefd[1], &num, sizeof(int));
		}
		//exec once a progNum
		if else (command == '@') {
			int num = atoi(param);
			execProg(num);
		}

		/* read on stdin */
		nbCharRd = sread(0, bufRd, BUFFERSIZE);
		command = bufRd[0];
		bufRd[nbCharRd-1] = '\0';
		param = bufRd+2;
	}

	ssigaction(SIGUSR1, sigusr1_handler);
	skill(timer, SIGUSR1);


	//close write
	ret = close(pipefd[1]);
	checkNeg(ret, "close error");

	return 0;
}


void sigusr1_handler() {
  end = 1;
}



//***************************************************************************
// TIMER
//***************************************************************************

void timer(void *arg1, void* arg2) {
	int *pipefd = arg1;
	int delay = *(int)arg2;
	int heartBit = -1;

	//close read
	int ret = close(pipefd[0]);
	checkNeg(ret, "close error");

	while (end == 0) {
		sleep(delay);
		swrite(pipefd[1], &heartBit, sizeof(int));
	}
	
	// close write
	int ret = close(pipefd[1]);
	checkNeg(ret, "close error");
}



//***************************************************************************
// RECUREXEC
//***************************************************************************
 
void recurExec(void *arg1, void* arg2, void* arg3) {
	int *pipefd = arg1;
	char* adr = *arg2;
	int port = *(int)arg3;

	int num;
	int progs[100];
	int nbProgs = 0;
	
	// close write
	int ret = close(pipefd[1]);
	checkNeg(ret, "close error");

	// read pipe
	int szIntRd = sread(pipefd[0], &num, sizeof(int));

	while(szIntRd > 0){
		if (num < 0) {
			for (int i = 0; i < nbProgs; ++i) {
				execProg(progs[i]);
			}			
		}
		else {
			//command * (add a progNum in RecurExec)
			progs[nbProgs] = num;
			nbProgs++;
		}
	}

	//close read
	int ret = close(pipefd[0]);
	checkNeg(ret, "close error");
}