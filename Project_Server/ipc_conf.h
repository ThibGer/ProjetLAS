#ifndef IPC_CONF_H

#include <stdbool.h>

#define IPC_CONF_H

#define PERM 0666

#define SHARED_MEMORY_KEY 370

#define SEM_KEY 369

#define MAX_NAME 20


typedef struct {
  int num;
  char name[MAX_NAME];
  bool errorCompil;
  int numberOfExecutions;
  int time;
} StructProgram;

typedef struct {
	int numberOfPrograms;
	StructProgram structProgram[1000];
} MainStruct;



#endif
