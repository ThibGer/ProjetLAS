#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "utils_v10.h"
#include "../communications.h"

#define BUFFERSIZE 300

volatile sig_atomic_t end = 0;




//upload à mettre dans client.c
void uploadFile(int sockfd, char* pathFile){
  int fd = sopen(pathFile,O_RDONLY,0100);

  char[1000] buffer;
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

	char* adr = argv[0];
	int port = atoi(argv[1]);
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
		if (command == '+'){
			//strChar rech filename in filePath;
		}
		//replace a C file to the server
		if (command == '.'){
			//split 2 param
			char* spaceAddress = strtok(param, ' ');
			char* filePath = spaceAddress+1;
			*spaceAddress = '\0';
			int num = atoi(param);

			char* fileName = strchr(filepath, int c'/')+1;
			char* file = //TODO;
			
			CommunicationServerClient msg;
			msg.num = num;
			msg.file = file;
			msg.nbCharFilename = strlen(fileName);
			msg.filename[0] = fileName;
			/*TODO: envoi struct(num, NULL, NULL) à server)*/;
		}
		//add a progNum in RecurExec
		if (command == '*'){
			int num = atoi(param);
			swrite(pipefd[1], &num, sizeof(int));
		}
		//exec once a progNum
		if (command == '@'){
			int num = atoi(param);
			CommunicationServerClient msg;
			msg.num = num;
			/*TODO: execution (envoi struct à server)*/;
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
			CommunicationServerClient msg;
			msg.num = num;
			/*TODO: execution (envoi struct à server)*/;
		}
		else {
			//command * (add a progNum in RecurExec)
			progs[nbProgs] = num;
			progNum++;
		}
	}

	//close read
	int ret = close(pipefd[0]);
	checkNeg(ret, "close error");
}