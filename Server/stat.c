#include <stdio.h>


#include "ipc_conf.h"
#include "../utils_v10.h"

//******************************************************************************
// MAIN FUNCTION
//******************************************************************************
int main (int argc, char *argv[]) {

	if( argc != 2 || atoi(argv[1]) < 0 || atoi(argv[1]) > 999 ){
		perror("Un argument au bon format est attendu.");
		exit(1);
	}

	int shid = sshmget(SHARED_MEMORY_KEY, sizeof(MainStruct), 0);
	int sid = sem_get(SEM_KEY, 2);

	sem_down0(sid);
 	MainStruct *s = sshmat(shid);
 	StructProgram prog = s->structProgram[atoi(argv[2])];
 	printf("%d",prog.num);
 	printf("%s",prog.name);
 	printf("%d",prog.errorCompil);
 	printf("%d",prog.numberOfExecutions);
 	printf("%d",prog.time);

 	sshmdt(s);
 	sem_up0(sid);
}
