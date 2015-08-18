#ifndef PACK_HEADER
#define PACK_HEADER

typedef struct  {
    char[11] cmd;
    int data;
}pck_Client;

typedef struct  {
    int return_val;
}pck_Server;

// Returns a client package to be sent to the server
pck_Client pck_NewClientPck(char * cmd, int data);

// Returns a server package to be sent to the client
pck_Server pck_NewServerPck(int return_val);

#ifdef PACK_IMPLEMENTATION

#include <string.h>

pck_Client pck_NewClientPck(char * cmd, int data) {
    pck_Client pck;
    strncpy(pck.cmd, cmd, 10);
    pck.cmd[10] = '\0';
    pck.data = data;
    return pck;
}

pck_Server pck_NewServerPck(int return_val) {
    pck_Server pck;
    pck.return_val = return_val;
    return pck;
}


#endif // PACK_IMPLEMENTATION

#endif // !PACK_HEADER
