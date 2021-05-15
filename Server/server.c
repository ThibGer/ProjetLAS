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


void saveFileHandler(void* arg1, void* arg2, void* arg3){
  int sockfd = *(int*)arg1;
  CommunicationClientServer clientMsg = *(CommunicationClientServer*)arg2;
  int shid = *(int*)arg3;
  char buffer[1000];

  int sid = sem_get(SEM_KEY, 2);
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
  char *path = "./CodeDirectory";
  char number[10];
  sprintf(number,"%d",numberOfPrograms);
  strcat(path,number);
  strcat(path,".c");
  int fd = sopen(path, O_WRONLY | O_APPEND | O_CREAT, 0200);
  
  while(sread(sockfd,&buffer,sizeof(buffer)) != 0){
    nwrite(fd,buffer,strlen(buffer));
  }
 
  s->numberOfPrograms ++;
  CommunicationServerClient serverMsg;
  serverMsg.isCompiled = 0;

  //Compilation
  chdir("./CodeDirectory");
  path = number;
  strcat(path,".c");

  int compil = execl("/usr/bin/gcc","gcc", "-o", number, path, NULL);
  if(compil < 0){
  	  prog.errorCompil = true;
  	  serverMsg.isCompiled = -1;
  }
  //Reponse du serveur
  serverMsg.num = numberOfPrograms;
  //serverMsg.message = récupérer la suite de caractères.



  swrite(sockfd, &serverMsg,sizeof(serverMsg));

  //TODO VOIR AVEC LE EXEC QUI QUITE LE PROG......
  sem_up0(sid);
  sshmdt(s);
  sclose(fd);


}

void execProgram(void* arg1){
    char* num = (char *)arg1;
    execl(num,num, NULL);
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
    if(&clientMsg.num == NULL && clientMsg.filename != NULL){
        void *arg1 = &newsockfd;
        void *arg2 = &clientMsg;
        void *arg3 = &shid;
        fork_and_run3(saveFileHandler,arg1,arg2,arg3);

        



        /*i. Le numéro associé au programme.
          ii. 0 si le programme compile, un nombre différent de 0 sinon.
          iii. Une suite de caractères qui correspond aux messages d’erreur du compilateur*/

    //Remplacer programme (.)
    } else if (&clientMsg.num != NULL && clientMsg.filename != NULL){
        
    //Executer programme (*,@)
    } else if (&clientMsg.num != NULL && clientMsg.filename == NULL){
        void *ptr = &clientMsg.num;
        fork_and_run1(execProgram,ptr);
        /*serverMsg.num = clientMsg.num;
        serverMsg.state;
        serverMsg.executionTime;
        serverMsg.returnCode;
        serverMsg.standardOutput;*/
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