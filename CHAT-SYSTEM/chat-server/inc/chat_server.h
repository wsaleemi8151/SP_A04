
int InitChatServer(void);
void *socketThread(void *);
void UpdateConnectedClientListOnDelete(int clSocket);
void *messageThread(void *dummy);
void ParseMessage(char *message, int clSocket, char *userId);

#define NO_OF_CLIENTS 10
#define MESSAGE_QUEUE_LENGTH 10

typedef struct _ConnectedClient
{
    pthread_t tid;
    char userId[10];
    // struct sockaddr *client_addr;
    int client_socket;
    struct sockaddr_in *client_addr;
} ConnectedClient;
