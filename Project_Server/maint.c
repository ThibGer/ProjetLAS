#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h> 
#include <sys/ipc.h>
#include <sys/sem.h>

#include "ipc_conf.h"
#include "../utils_v10.h"

//******************************************************************************
// MAIN FUNCTION
//******************************************************************************
int main (int argc, char *argv[]) {
  if(argc < 2){
    perror("Un argument est attendu.");
    exit(EXIT_FAILURE);
  } 

  if(atoi(argv[1]) == 1){
    // shared memory
    sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), IPC_CREAT | PERM);
    // semaphore
    sem_create(SEM_KEY, 1, PERM, 1);

  } else if(atoi(argv[1]) == 2){
    int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), IPC_CREAT | PERM);
    int sid = sem_get(SEM_KEY, 1);

    sshmdelete(shid);
    sem_delete(sid);

  } else if(atoi(argv[1]) == 3){
    if(argc != 3){
      perror("Une durée est nécessaire afin de réserver la mémoire de façon exclusive.");
      exit(EXIT_FAILURE);
    }
    int sid = sem_get(SEM_KEY, 1);
    sem_down0(sid);
    sleep(atoi(argv[2])); 
    sem_up0(sid);

  }
  exit(EXIT_SUCCESS);
}
