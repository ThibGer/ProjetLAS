#include <sys/types.h>
#include <sys/shm.h> 
#include <sys/ipc.h>





#include "ipc_conf.h"
#include "../utils_v10.h"

//******************************************************************************
// MAIN FUNCTION
//******************************************************************************
int main (int argc, char *argv[]) {
  int shid;
  int sid;
  if(argc < 2){
    perror("Un argument est attendu.");
  } 
  // exit?
  if(argv[1] == 1){
    // shared memory
    shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), IPC_CREAT | PERM);

    // semaphores
    sid = sem_create(SEM_KEY, 2);

  } else if(argv[1] == 2){
    sshmdelete(shid);
    sem_delete(sid);

  } else if(argv[1] == 3){
    if(argc != 3){
      perror("Une durée est nécessaire afin de réserver la mémoire de façon exclusive.");
    }
    //exit?
    sem_down0(sid);
    sleep(argv[2]); 
    sem_up0(sid);

  }
  
}
