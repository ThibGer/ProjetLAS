#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

typedef struct {
    int num;
    int nbCharFilename;
    char filename[255];
} CommunicationClientServer;

typedef struct {
    int num;
    int isCompiled;
    int state;
    int executionTime;
    int returnCode;
    char message[255];
} CommunicationServerClient;

#endif
