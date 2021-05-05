

#include "../utils_v10.h"

//******************************************************************************
// MAIN FUNCTION
//******************************************************************************
int main (int argc, char *argv[]) {

	if( agc != 1 || argv[1] < 0 || argv[1] > 999 ){
		perror("Un argument au bon format est attendu.");
	}

	int shid = sshmget(MEMORY_KEY, 1000 * sizeof(MainStruct), 0);
	int sid = sem_get(SEM_KEY, 2);

	sem_down0(sid);
 	StructProgram* s = sshmat(shid);
 	StructProgram prog = s[argv[1]];
 	printf(prog.name);
 	printf(prog.name);
 	printf(prog.errorCompil);
 	printf(prog.numberOfExecutions);
 	printf(prog.time);

 	sshmdt(shid);
 	sem_up0(sid);
}
