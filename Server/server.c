#include <stdio.h>
#include <stdlib.h>

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

  MainStruct *s = sshmat(shid);
  int numberOfPrograms = s->numberOfPrograms;
  StructProgram prog = s->structProgram[numberOfPrograms];
  prog.num = numberOfPrograms;
  strcpy(prog.name,clientMsg.filename);
  prog.errorCompil = false;
  prog.numberOfExecutions = 0;
  prog.time = 0;

  int fd = sopen("./CodeDirectory/"+numberOfPrograms+".c", O_WRONLY | O_APPEND | O_CREAT, 0200);
  
  while(sread(sockfd,&buffer,sizeof(buffer)) != 0){
    nwrite(fd,buffer,strlen(buffer));
  }
 
  s->numberOfPrograms ++;
  sem_up0(sid);
  sshmdt(s);
  sclose(fd);
}



void socketHandler(void* arg1) {
    //Get shared memory
    int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), 0);

    int newsockfd = *(int *)arg1;
    printf("Numéro du socket dans fils : %d\n",newsockfd);
    CommunicationClientServer clientMsg;
    //CommunicationServerClient serverMsg;
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
    } else if (&clientMsg.num != NULL && clientMsg.num != -2 && clientMsg.file != NULL && clientMsg.filename != NULL){
        
    //Executer programme (*,@)
    } else if (&clientMsg.num != NULL && clientMsg.file == NULL && clientMsg.filename == NULL){

    } 

}

int main (int argc, char ** argv){
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