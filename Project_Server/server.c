#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "ipc_conf.h"
#include "../utils_v10.h"
#include "../communications.h"

#define BACKLOG 5
#define PATHSIZE 25
#define PROG_NAME_SIZE 5 

// PRE:  arg1: the number of prog to be compiled
//       arg2: pipe
// POST: on success, generate the executable in ./CodeDirectory 
//       with the prog number
//       on failure, EXIT_FAILURE
void compilHandler(void* arg1,void* arg2){
  char *numProg = (char*) arg1;
  int *pipefd = arg2; 

  sclose(pipefd[0]);

  chdir("./CodeDirectory");
  char path[PATHSIZE];
  strcpy(path,numProg);
  strcat(path,".c");

  dup2(pipefd[1],STDERR_FILENO);
  execl("/usr/bin/gcc","gcc", "-o", numProg, path, NULL);
  sclose(pipefd[1]);
  exit(EXIT_FAILURE);
}

// PRE: arg1 : a void pointer of a name of an executable
//      arg2 : pipe
// POST: execute the program
// RES:  return exit code of program in case of success
//       or EXIT_FAILURE code in case of failure
void execHandler(void* arg1,void* arg2){
  char *progName = (char *)arg1;
  int *pipefd = arg2; 

  sclose(pipefd[0]);
  int saved_stdout = dup(STDOUT_FILENO);
  dup2(pipefd[1],STDOUT_FILENO);

  chdir("./CodeDirectory");
  execl(progName,progName, NULL);
  sclose(pipefd[1]);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);
  exit(EXIT_FAILURE);
}


// PRE: pipefd : pipe
//      sockfd : a socket file descriptor
// POST: read and return the text sent through the socket
void sendMessage(int *pipefd,int sockfd) {
  char buffer[BUFFERSIZE];
  int n = sread(pipefd[0],buffer,BUFFERSIZE * sizeof(char));
  while(n > 0){
    nwrite(sockfd,buffer,n * sizeof(char));
    n = sread(pipefd[0],buffer,BUFFERSIZE * sizeof(char));
  }
}



// PRE:  num: the number of prog to be save
//       sockfd: a socket file descriptor
// POST: on success generate the file in ./CodeDirectory with the prog number
//       on failure, print Error OPEN.
void readDataAndSave(int num, int sockfd){
  char path[PATHSIZE] = "./CodeDirectory/";
  char number[PROG_NAME_SIZE];
  sprintf(number,"%d",num);

  strcat(path,number);
  strcat(path,".c");
  int fd = sopen(path, O_WRONLY | O_TRUNC | O_CREAT, PERM);
  
  char buffer[BUFFERSIZE];
  int n = sread(sockfd,buffer,BUFFERSIZE * sizeof(char));
  while(n > 0){
    nwrite(fd,buffer,n * sizeof(char));
    n = sread(sockfd,buffer,BUFFERSIZE * sizeof(char));
  }
  sclose(fd);
}

// PRE:  num: the number of prog to be compiled
//       sockfd: a socket file descriptor
//       prog: the shared memory of the program to be compiled
// POST: on success, sends the client a success message.
//       on failure, sends the client an error message.
void compilation(int num, int sockfd, StructProgram *prog){
  CommunicationServerClient serverMsg;
  serverMsg.num = num;
  char numChar[PROG_NAME_SIZE];
  sprintf(numChar, "%d", num);
  
  int pipefd[2];
  spipe(pipefd);

  void *numProg = &numChar;
  
  pid_t pidCompil = fork_and_run2(compilHandler,numProg,pipefd);
  sclose(pipefd[1]);

  int status;
  swaitpid(pidCompil,&status,0);

  if(WEXITSTATUS (status) != 0){
    //Error 
    serverMsg.state = -1;
    prog->errorCompil = true;
  }else{
    //Good
    serverMsg.state = 0;
    prog->errorCompil = false;
  }
  
  nwrite(sockfd, &serverMsg,sizeof(serverMsg));

  if(WEXITSTATUS (status) != 0){
    sendMessage(pipefd,sockfd);
  }

  sclose(pipefd[0]);
}


// PRE:  sockfd: a socket file descriptor
//       clientMsg: communication client server to be sent
//       shid: id of shared memory
// POST: createFile
void createFile(int sockfd, CommunicationClientServer clientMsg, int shid){
  int sid = sem_get(SEM_KEY, 1);
  sem_down0(sid);

  //Stock dans la m??moire partag??e
  MainStruct *s = sshmat(shid);
  int numberOfPrograms = s->numberOfPrograms;
  StructProgram prog;
  prog.num = numberOfPrograms;
  strcpy(prog.name,clientMsg.filename);

  prog.errorCompil = false;
  prog.numberOfExecutions = 0;
  prog.time = 0;

  readDataAndSave(numberOfPrograms,sockfd);

  compilation(numberOfPrograms,sockfd,&prog);

  s->structProgram[numberOfPrograms] = prog;
  s->numberOfPrograms ++;

  sshmdt(s);
  sem_up0(sid);
}

// PRE:  sockfd: a socket file descriptor
//       clientMsg: communication client server to be sent
//       shid: id of shared memory
//POST:  replaceFile
void replaceFile(int sockfd,CommunicationClientServer clientMsg, int shid){
  int sid = sem_get(SEM_KEY, 1);
  sem_down0(sid);
  MainStruct *s = sshmat(shid);
  StructProgram prog = s->structProgram[clientMsg.num];
  strcpy(prog.name,clientMsg.filename);

  readDataAndSave(clientMsg.num,sockfd);
  
  compilation(clientMsg.num,sockfd,&prog);

  s->structProgram[clientMsg.num] = prog;

  sshmdt(s);
  sem_up0(sid);
}

// PRE: serverMsg : a pointer of CommunicationServerClient struct
//      newsockfd : a valid client socket
// POST: set executionTime and returnCode to -1
//       write empty string on client socket
void progNotExistOrNotCompile(CommunicationServerClient* serverMsg, int newsockfd){
  serverMsg->executionTime = -1;
  serverMsg->returnCode = -1;
}

// PRE:  newsockfd: a socket file descriptor
//       clientMsg: communication client server to be sent
//       shid: id of shared memory
//POST:  execFile
void execFile(int newsockfd, CommunicationClientServer clientMsg, int shid){
  int pipefd[2];
  chdir("./CodeDirectory");
  struct stat statStruct;
  char progName[PROG_NAME_SIZE];
  char progNum[3];
  sprintf(progName, "%d", clientMsg.num);
  sprintf(progNum, "%d", clientMsg.num);
  strcat(progName,".c");
  int fileExist = stat(progName, &statStruct);
  MainStruct *s = sshmat(shid);
  StructProgram prog = s->structProgram[clientMsg.num];
  CommunicationServerClient serverMsg;
  int sid = sem_get(SEM_KEY, 1);
  sem_down0(sid);
    //If file doesn't exist
  if(fileExist < 0) {
    serverMsg.state = -2;
    progNotExistOrNotCompile(&serverMsg,newsockfd);
    //If file doesn't compile
  } else if(prog.errorCompil){
    serverMsg.state = -1;
    progNotExistOrNotCompile(&serverMsg,newsockfd);
  } else {
    void *ptr = &progNum;
    struct timeval t1;
    struct timeval t2;
    gettimeofday(&t1, NULL);
      //Redirect StdOut to socket   
    spipe(pipefd);
    pid_t child = fork_and_run2(execHandler,ptr,pipefd);
    sclose(pipefd[1]);
    
    int status;

    swaitpid(child, &status, 0);

    gettimeofday(&t2, NULL);
    int executionTime = (int)(t2.tv_usec - t1.tv_usec);
      //If program didn't end correctly
    if(WEXITSTATUS (status) == EXIT_FAILURE){
      serverMsg.state = 0;
    } else {
      prog.numberOfExecutions ++;
      prog.time += executionTime;
      s->structProgram[clientMsg.num] = prog;
      serverMsg.state = 1;
    }
    serverMsg.returnCode = WEXITSTATUS (status);
    serverMsg.executionTime = executionTime;

  }
  sshmdt(s);
  sem_up0(sid);
  serverMsg.num = clientMsg.num;
  nwrite(newsockfd, &serverMsg,sizeof(serverMsg));
  sendMessage(pipefd,newsockfd);

  sclose(pipefd[0]);
}

// PRE: arg1 : a void pointer of a client socket
// POST: Handle the socket client
void socketHandler(void* arg1) {
  //Get shared memory
  int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), 0);

  int newsockfd = *(int *)arg1;
  CommunicationClientServer clientMsg;
  sread(newsockfd,&clientMsg,sizeof(clientMsg));
  //Add file (+)
  if(clientMsg.num == -1 && clientMsg.nbCharFilename != -1){
    createFile(newsockfd,clientMsg,shid);
  //Replace file (.)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename != -1){
    replaceFile(newsockfd,clientMsg,shid);
  //Execute file (*,@)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename == -1){
    execFile(newsockfd,clientMsg,shid);
  }
  int s = shutdown(newsockfd,SHUT_WR); 
  checkNeg(s, "ERROR SHUTDOWN");
  sclose(newsockfd);
}




// PRE:  ServerPort: a valid port number
// POST: on success bind a socket to 0.0.0.0:port and listen to it
//       return socket file descriptor
//       on failure, displays error cause and quits the program
int initSocketServer(int port) {
  int sockfd  = ssocket();

  /* no socket error */

  sbind(port, sockfd);
  
  /* no bind error */
  slisten(sockfd, BACKLOG);

  // setsockopt -> to avoid Address Already in Use
  int option = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
  
  /* no listen error */
  return sockfd;
}


int main (int argc, char ** argv){
  
  if(argc != 2){
    perror("Le port est attendu");
    exit(EXIT_FAILURE);
  }
  int port = atoi(argv[1]);
  int sockfd, newsockfd;

  sockfd = initSocketServer(port);
  printf("Le serveur tourne sur le port : %i \n",port);

  while (1){
    printf("Le serveur attend une connexion\n");

    newsockfd = saccept(sockfd);
      //If socket has been accepted
    if (newsockfd > 0){
      void *ptr = &newsockfd;
      fork_and_run1(socketHandler,ptr);
    }
  }
}