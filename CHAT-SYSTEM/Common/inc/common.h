
#include <sys/socket.h>
#define INPUT_MESG_LENGTH 80
#define TRANSMISSION_MESG_LENGTH 40

typedef struct _ChatMessage
{
    char *userId;
    // struct sockaddr *client_addr;
    int client_socket;
    char message[INPUT_MESG_LENGTH];
} ChatMessage;

