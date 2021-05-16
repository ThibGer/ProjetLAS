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
	int sid = sem_get(SEM_KEY, 1);
	sem_down0(sid);
	MainStruct *s = sshmat(shid);
	StructProgram prog = s->structProgram[atoi(argv[1])];

	printf("Numero du programme : %d\n",prog.num);
	printf("Nom du programme : %s\n",prog.name);
	printf("Erreur de compilation : %d\n",prog.errorCompil);
	printf("Nombre d'éxécutions du programme : %d\n",prog.numberOfExecutions);
	printf("Temps cumulé (microsecondes) : %d\n",prog.time);

	sshmdt(s);
	sem_up0(sid);
}
