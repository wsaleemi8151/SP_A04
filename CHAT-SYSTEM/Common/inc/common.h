
#include <sys/socket.h>

int checkPrefix(char *pre, char *str);

#define INPUT_MESG_LENGTH 80
#define TRANSMISSION_MESG_LENGTH 67
#define USER_ID_LENGTH 10

static char userPrefix[] = "-user";

typedef struct _ChatMessage
{
    char userId[10];
    // struct sockaddr *client_addr;
    int client_socket;
    char message[INPUT_MESG_LENGTH];
} ChatMessage;

typedef struct _ClientSocket
{
    char userId[10];
    int clientSocket;
} ClientSocketStruct;
