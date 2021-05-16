#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#define BUFFERSIZE 300

typedef struct {
    int num;
    int nbCharFilename;
    char filename[255];
} CommunicationClientServer;

typedef struct {
    int num;
    int state;
    int executionTime;
    int returnCode;
} CommunicationServerClient;

#endif
