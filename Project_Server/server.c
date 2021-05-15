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
#define SERVER_PORT 9502
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
  //CommunicationServerClient serverMsg = *(CommunicationServerClient*)arg1;
  char *numProg = (char*)arg2;

  //Compilation
  chdir("./CodeDirectory");
  
  char path[PATH_SIZE];
  strcpy(path,numProg);
  strcat(path,".c");

  execl("/usr/bin/gcc","gcc", "-o", numProg, path, NULL);
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
  StructProgram prog = s->structProgram[numberOfPrograms];
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
        printf("le char%c\n",buffer[i]);
        buffer[i] = '\0';
        i --;
      }
      
    }
    nwrite(fd,buffer,strlen(buffer));

  }
  s->numberOfPrograms ++;
  CommunicationServerClient serverMsg;
  //serverMsg.isCompiled = 0;
  
  serverMsg.num = numberOfPrograms;
  


  //COMPILATION
  void *numProg = &number;
  fork_and_run1(compilHandler,numProg);

  //TODO dans serverMsg
  //0 si le programme compile, un nombre différent de 0 sinon. => a faire prendre le status code de compilHandler
  //Une suite de caractères qui correspond aux messages d’erreur du compilateur.


  //TODO dans prog (memoire partagee)
  //prog.errorCompil => prendre status code de compil Handler



  swrite(sockfd, &serverMsg,sizeof(serverMsg));

  sshmdt(s);
  sem_up0(sid);
  sclose(fd);
}

void execProgram(void* arg1){
  int num = *(int *)arg1;
  char progName[4];
  sprintf(progName, "%d", num);
  chdir("./CodeDirectory");
  int ret = execl(progName,progName, NULL);
  printf("Retour : %d\n",ret);
  exit(ret);
}


void socketHandler(void* arg1) {
  //Get shared memory
  int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), 0);

  int newsockfd = *(int *)arg1;
  printf("Numéro du socket dans fils : %d\n",newsockfd);
  CommunicationClientServer clientMsg;
  CommunicationServerClient serverMsg;
  sread(newsockfd,&clientMsg,sizeof(clientMsg));
  //Ajout fichier (+)
  if(clientMsg.num == -1 && clientMsg.nbCharFilename != -1){
    printf("On ajoute\n");
      void *arg1 = &newsockfd;
      void *arg2 = &clientMsg;
      void *arg3 = &shid;
      fork_and_run3(saveFileHandler,arg1,arg2,arg3);

      /*i. Le numéro associé au programme.
        ii. 0 si le programme compile, un nombre différent de 0 sinon.
        iii. Une suite de caractères qui correspond aux messages d’erreur du compilateur*/

  //Remplacer programme (.)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename != -1){
    printf("On Remplace\n");

  //Executer programme (*,@)
  } else if (clientMsg.num != -1 && clientMsg.nbCharFilename == -1){
    printf("On execute\n");
    printf("Num prog %d\n",clientMsg.num);
    void *ptr = &clientMsg.num;
    struct timeval t1;
    struct timeval t2;
    gettimeofday(&t1, NULL);
    pid_t child = fork_and_run1(execProgram,ptr);

    int status;
    /* pid renvoyé par le wait */
    swaitpid(child, &status, 0);

    gettimeofday(&t2, NULL);
    int executionTime = (int)(t2.tv_usec - t1.tv_usec);
    printf("Temps d'execution : %d \n",executionTime);
    if ( WIFEXITED(status) ){
      int exit_status = WEXITSTATUS(status);        
        printf("Exit status of the child was %d\n",exit_status);
    }

    serverMsg.num = clientMsg.num;
    //serverMsg.state;
    serverMsg.executionTime = executionTime;
    //serverMsg.returnCode;
    //serverMsg.message;
    swrite(newsockfd, &serverMsg,sizeof(serverMsg));
    } 


}

/*void testExecHandler () {
  printf("testExecHandler\n");
  chdir("./CodeDirectory");
  int ret = execl("helloWorld", "helloWord", NULL);
  printf("Retour : %d\n",ret);
  exit(ret);
}*/

int main (int argc, char ** argv){
  //char s[100];
  //chdir("./CodeDirectory");
  //sexecl("/usr/bin/gcc","gcc", "-o", "helloWorld", "helloWorld.c", NULL);
  // printing current working directory
  //printf("%s\n", getcwd(s, 100));
  /*struct timeval t1;
  struct timeval t2;
  gettimeofday(&t1, NULL);
  pid_t child = fork_and_run0(testExecHandler);

  int status;*/
  /* pid renvoyé par le wait */
  /*swaitpid(child, &status, 0);

  gettimeofday(&t2, NULL);
  int executionTime = (int)(t2.tv_usec - t1.tv_usec);
  printf("Temps d'execution : %d \n",executionTime);
  if ( WIFEXITED(status) ){
    int exit_status = WEXITSTATUS(status);        
      printf("Exit status of the child was %d\n",exit_status);
  }*/

  int sockfd, newsockfd;

  //Pour plus tard : remplacer la constante SERVER_PORT par le premier argument
  sockfd = initSocketServer(SERVER_PORT);
  printf("Le serveur tourne sur le port : %i \n",SERVER_PORT);

  while (1){
      printf("Le serveur attend une connexion\n");

      newsockfd = saccept(sockfd);
      //Si le socket a bien été accepté
      if (newsockfd > 0 ){
          printf("Numéro du socket dans parent : %d\n",newsockfd);
          void *ptr = &newsockfd;
          pid_t childID = fork_and_run1(socketHandler,ptr);
          printf("Id du child : %d\n",childID);
      }
  }
}