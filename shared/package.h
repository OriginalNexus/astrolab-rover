#ifndef PACK_HEADER
#define PACK_HEADER

#define CMD_FORWARD         1
#define CMD_BACKWARD        2
#define CMD_LEFT            3
#define CMD_RIGHT           4
#define CMD_CLAW            5
#define CMD_STEP_STEP       6
#define CMD_STEP_RELEASE    7
#define CMD_DISCONNECT      99
#define CMD_EXIT            999

typedef struct  {
    int cmd;
    int data;
}pck_Client;

typedef struct  {
    int return_val;
}pck_Server;

// Returns a client package to be sent to the server
pck_Client pck_NewClientPck(int cmd, int data);

// Returns a server package to be sent to the client
pck_Server pck_NewServerPck(int return_val);

#ifdef PACK_IMPLEMENTATION

pck_Client pck_NewClientPck(int cmd, int data) {
    pck_Client pck;
    pck.cmd = cmd;
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
