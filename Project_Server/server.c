#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "ipc_conf.h"
#include "../utils_v10.h"
#include "../communications.h"

#define BACKLOG 5
#define PATH_SIZE 30

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

void compilHandler(void* arg2){
  char *numProg = (char*)arg2;
  chdir("./CodeDirectory");
  
  char path[PATH_SIZE];
  strcpy(path,numProg);
  strcat(path,".c");

  execl("/usr/bin/gcc","gcc", "-o", numProg, path, NULL);
  exit(EXIT_FAILURE);
}


void saveFileHandler(void* arg1, void* arg2, void* arg3){
  int sockfd = *(int*)arg1;
  CommunicationClientServer clientMsg = *(CommunicationClientServer*)arg2;
  int shid = *(int*)arg3;
  char buffer[1000];

  int sid = sem_get(SEM_KEY, 1);
  sem_down0(sid);

  //Stock dans la mémoire partagée
  MainStruct *s = sshmat(shid);
  int numberOfPrograms = s->numberOfPrograms;
  StructProgram prog;
  prog.num = numberOfPrograms;
  strcpy(prog.name,clientMsg.filename);

  prog.errorCompil = false;
  prog.numberOfExecutions = 0;
  prog.time = 0;
  char path[PATH_SIZE] = "./CodeDirectory/";
  char number[6];

  sprintf(number,"%d",numberOfPrograms);

  strcat(path,number);
  strcat(path,".c");
  int fd = sopen(path, O_WRONLY | O_APPEND | O_CREAT, 0666);
  
  while(sread(sockfd,&buffer,sizeof(buffer)) != 0){
    // TODO A voir si c'est toujours fonctionnel pour de grands progs. peut etre != 999
    if(strlen(buffer) != 1000){
      int i = strlen(buffer);
      while(buffer[i] != '}'){
        buffer[i] = '\0';
        i --;
      }
      
    }
    nwrite(fd,buffer,strlen(buffer));

  }
  s->numberOfPrograms ++;



  CommunicationServerClient serverMsg;
  serverMsg.num = numberOfPrograms;
  
  //COMPILATION
  void *numProg = &number;
  int dupFd = dup2(sockfd,1);
  pid_t pidCompil = fork_and_run1(compilHandler,numProg);
  int status;
  swaitpid(pidCompil,&status,0);
  sclose(dupFd);
  if(WEXITSTATUS (status) != 0){
    //Error 
    serverMsg.state = -1;
    prog.errorCompil = true;
  }else{
    //Good
    serverMsg.state = 0;
    prog.errorCompil = false;
  }

  s->structProgram[numberOfPrograms] = prog;




  swrite(sockfd, &serverMsg,sizeof(serverMsg));

  sshmdt(s);
  sem_up0(sid);
  sclose(fd);
}

void execHandler(void* arg1){
  char *progName = (char *)arg1;
  chdir("./CodeDirectory");
  execl(progName,progName, NULL);
  exit(EXIT_FAILURE);
}

void progNotExistOrNotCompile(CommunicationServerClient* serverMsg, int newsockfd){
    serverMsg->executionTime = -1;
    serverMsg->returnCode = -1;
    char fileDoesntExist[1] = "";
    nwrite(newsockfd,fileDoesntExist,sizeof(fileDoesntExist));
}

void socketHandler(void* arg1) {
  //Get shared memory
  int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), 0);

  int newsockfd = *(int *)arg1;
  CommunicationClientServer clientMsg;
  CommunicationServerClient serverMsg;
  sread(newsockfd,&clientMsg,sizeof(clientMsg));
  //Ajout fichier (+)
  if(clientMsg.num == -1 && clientMsg.nbCharFilename != -1){
    printf("ADD FILE\n");
      void *arg1 = &newsockfd;
      void *arg2 = &clientMsg;
      void *arg3 = &shid;
      fork_and_run3(saveFileHandler,arg1,arg2,arg3);
  //Remplacer programme (.)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename != -1){
    printf("On Remplace\n");

  //Executer programme (*,@)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename == -1){
    printf("On execute\n");
    int dupFd = -1;
    chdir("./CodeDirectory");
    struct stat statStruct;
    char progName[5];
    char progNum[3];
    sprintf(progName, "%d", clientMsg.num);
    sprintf(progNum, "%d", clientMsg.num);
    strcat(progName,".c");
    int fileExist = stat(progName, &statStruct);
    MainStruct *s = sshmat(shid);
    StructProgram prog = s->structProgram[clientMsg.num];
    //Si le fichier n'existe pas
    if(fileExist < 0) {
      printf("Fichier existe pas\n");
      serverMsg.state = -2;
      progNotExistOrNotCompile(&serverMsg,newsockfd);
    //Si le fichier ne compile pas
    } else if(prog.errorCompil){
      printf("Fichier compile pas\n");
      serverMsg.state = -1;
      progNotExistOrNotCompile(&serverMsg,newsockfd);
    } else {
      printf("Compile et existe\n");
      void *ptr = &progNum;
      struct timeval t1;
      struct timeval t2;
      gettimeofday(&t1, NULL);
      //Redirige StdOut vers le socket
      dupFd = dup2(newsockfd,1);
      pid_t child = fork_and_run1(execHandler,ptr);
      sclose(dupFd);

      int status;
      /* pid renvoyé par le wait */
      swaitpid(child, &status, 0);

      gettimeofday(&t2, NULL);
      int executionTime = (int)(t2.tv_usec - t1.tv_usec);
      //Si le programme ne s'est pas terminé correctement
      if(WEXITSTATUS (status) == EXIT_FAILURE){
        serverMsg.state = 0;
      } else {
        int sid = sem_get(SEM_KEY, 1);
        sem_down0(sid);
        //On augmente le nombre d'executions et d'execution Time
        prog.numberOfExecutions ++;
        prog.time += executionTime;
        s->structProgram[clientMsg.num] = prog;
        sem_up0(sid);
        serverMsg.state = 1;
      }
      serverMsg.returnCode = WEXITSTATUS (status);
      serverMsg.executionTime = executionTime;

    }
      serverMsg.num = clientMsg.num;
      nwrite(newsockfd, &serverMsg,sizeof(serverMsg));
      //if(dupFd != -1)
        //sclose(dupFd);
    }


}


int main (int argc, char ** argv){
  
  if(argc != 2){
		perror("Le port est attendu");
		exit(EXIT_FAILURE);
	}
  int port = atoi(argv[1]);
  int sockfd, newsockfd;

  //Pour plus tard : remplacer la constante SERVER_PORT par le premier argument
  sockfd = initSocketServer(port);
  printf("Le serveur tourne sur le port : %i \n",port);

  while (1){
      printf("Le serveur attend une connexion\n");

      newsockfd = saccept(sockfd);
      //Si le socket a bien été accepté
      if (newsockfd > 0 ){
          void *ptr = &newsockfd;
          fork_and_run1(socketHandler,ptr);
      }
  }
}