#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

typedef struct {
    int num;
    char* file;
    int nbCharFilename;
    char filename[255];
} CommunicationClientServer;

typedef struct {
    int num;
    int isCompiled;
    char errorMessage[255];
    int state;
    int executionTime;
    int returnCode;
    char standardOutput[255];
} CommunicationServerClient;

#endif
