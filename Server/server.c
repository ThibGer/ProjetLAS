#include <stdio.h>
#include <stdlib.h>

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


void socketHandler(void* arg1) {
    int newsockfd = *(int *)arg1;
    printf("Numéro du socket dans fils : %d\n",newsockfd);
    CommunicationClientServer msg;
    sread(newsockfd,&msg,sizeof(msg));
    printf("message du client : %s\n",msg);

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