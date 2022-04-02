/*
 * tcpip-server.c
 *
 * This is a sample socket server using threads
 * to requests on port 5000
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

#include "../inc/chat_server.h"
#include "../../Common/inc/common.h"

#define PORT 5000

// global variable to keep count of the number of clients ...
static int MessageQueueCount = 0;

static pthread_t messageThreadId;
static int exitMessageThread = 0;
ChatMessage MessageQueue[MESSAGE_QUEUE_LENGTH];
ConnectedClient ConnectedClientsList[NO_OF_CLIENTS];
static int ConnectedClientsCount = 0;

int main(void)
{
    return InitChatServer();
}

int InitChatServer(void)
{

    int server_socket, client_socket;
    int client_len;
    struct sockaddr_in client_addr, server_addr;
    int len, i;
    int whichClient;

    /*
     * obtain a socket for the server
     */

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("[SERVER] : socket() FAILED\n");
        return 1;
    }
    printf("[SERVER] : socket() successful\n");

    /*
     * initialize our server address info for binding purposes
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[SERVER] : bind() FAILED\n");
        close(server_socket);
        return 2;
    }
    printf("[SERVER] : bind() successful\n");

    /*
     * start listening on the socket
     */
    if (listen(server_socket, 5) < 0)
    {
        printf("[SERVER] : listen() - FAILED.\n");
        close(server_socket);
        return 3;
    }
    printf("[SERVER] : listen() successful\n");

    // Thread to dispatch received messages to all clients
    if (pthread_create(&messageThreadId, NULL, messageThread, (void *)&client_socket))
    {
        printf("[SERVER] : messageThread() FAILED\n");
        fflush(stdout);
        return 5;
    }

    /*
     * for this server, run an endless loop that will
     * accept incoming requests from a remote client.
     * the server will create a thread to handle the
     * request, and the parent will continue to listen for the
     * next request - up to 3 clients
     */
    while (ConnectedClientsCount < NO_OF_CLIENTS)
    {
        printf("[SERVER] : Ready to accept()\n");
        fflush(stdout);

        /*
         * accept a packet from the client
         */
        client_len = sizeof(client_addr);
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            printf("[SERVER] : accept() FAILED\n");
            fflush(stdout);
            return 4;
        }

        printf("[SERVER] : received a packet from CLIENT-%02d\n", ConnectedClientsCount);
        fflush(stdout);

        // reading user id
        char userId[10];

        char buffer[INPUT_MESG_LENGTH];
        int numBytesRead = read(client_socket, buffer, INPUT_MESG_LENGTH);
        if (checkPrefix(userPrefix, buffer) == 0)
        {
            size_t firstMesgLen = strlen(buffer);
            size_t userPrefixLen = strlen(userPrefix);
            strncpy(userId, &buffer[userPrefixLen], firstMesgLen - userPrefixLen);
        }
        else
        {
            printf("[SERVER] : Invalid User Id\n");
            fflush(stdout);
            return 5;
        }
        /*
         * rather than fork and spawn the execution of the command within a
         * child task - let's take a look at something else we could do ...
         *
         * ... we'll create a thread of execution within this task to take
         * care of executing the task.  We'll be looking at THREADING in a
         * couple modules from now ...
         */

        ClientSocketStruct clientSocketSt;
        clientSocketSt.clientSocket = client_socket;

        memset(clientSocketSt.userId, 0, 10);
        strcpy(clientSocketSt.userId, userId);

        if (pthread_create(&(ConnectedClientsList[ConnectedClientsCount].tid), NULL, socketThread, (void *)&clientSocketSt))
        {
            printf("[SERVER] : pthread_create() FAILED\n");
            fflush(stdout);
            return 5;
        }
        else
        {
            ConnectedClientsList[ConnectedClientsCount].client_socket = client_socket;
            ConnectedClientsList[ConnectedClientsCount].client_addr = &client_addr;
            strcpy(ConnectedClientsList[ConnectedClientsCount].userId, userId);
            ConnectedClientsCount++;

            // strcpy(ConnectedClientsList[ConnectedClientsCount].userId, "");
        }

        printf("[SERVER] : pthread_create() successful for CLIENT-%02d\n", ConnectedClientsCount);
        fflush(stdout);
    }

    printf("[SERVER] : messageThread() successful");
    fflush(stdout);

    // once we reach No of clients - let's go into a busy "join" loop waiting for
    // all of the clients to finish and join back up to this main thread
    printf("\n[SERVER] : Now we wait for the threads to complete ... \n");
    for (i = 0; i < NO_OF_CLIENTS; i++)
    {
        int joinStatus = pthread_join(ConnectedClientsList[ConnectedClientsCount].tid, (void *)(&whichClient));
        if (joinStatus == 0)
        {
            printf("\n[SERVER] : received QUIT command from CLIENT-%02d (joinStatus=%d)\n", whichClient, joinStatus);
        }
    }

    // exiting message thread
    exitMessageThread = 1;
    int joinStatus = pthread_join(messageThreadId, (void *)(&whichClient));
    if (joinStatus == 0)
    {
        printf("\n[SERVER] : Message thread completed");
    }

    printf("\n[SERVER] : All clients have returned - exiting ...\n");
    close(server_socket);
    return 0;
}

//
// Socket handler - this function is called (spawned as a thread of execution)
//

void *socketThread(void *_clientSocketSt)
{
    // used for accepting incoming command and also holding the command's response
    char buffer[INPUT_MESG_LENGTH];
    char message[INPUT_MESG_LENGTH];
    int sizeOfRead;
    int timeToExit;
    int numBytesRead;

    // remap the clientSocket value (which is a void*) back into an INT
    ClientSocketStruct clientSocketSt = *((ClientSocketStruct *)_clientSocketSt);
    int clSocket = clientSocketSt.clientSocket;

    /* Clear out the input Buffer */
    memset(buffer, 0, INPUT_MESG_LENGTH);

    // increment the numClients
    int iAmClient = ConnectedClientsCount; // assumes that another connection from another client
                                           // hasn't been created in the meantime

    numBytesRead = read(clSocket, buffer, INPUT_MESG_LENGTH);

    while (strcmp(buffer, "quit") != 0)
    {
        memset(message, 0, INPUT_MESG_LENGTH);
        /* we're actually not going to execute the command - but we could if we wanted */
        sprintf(message, "[SERVER (Thread-%02d)] : %s Received %d bytes - command - %s\n", iAmClient, clientSocketSt.userId, numBytesRead, buffer);

        if (MessageQueueCount < MESSAGE_QUEUE_LENGTH)
        {
            // Update message queue when a message received
            printf("Client Socket: %d\n", clSocket);
            MessageQueue[MessageQueueCount].client_socket = clSocket;
            strcpy(MessageQueue[MessageQueueCount].message, message);
            strcpy(MessageQueue[MessageQueueCount].userId, clientSocketSt.userId);
            MessageQueueCount++;
        }
        else
        {
            printf("\n[SERVER] : Message Queue already full ...\n");
        }
        // clear out and get the next command and process
        memset(buffer, 0, INPUT_MESG_LENGTH);
        numBytesRead = read(clSocket, buffer, INPUT_MESG_LENGTH);
    }
    close(clSocket);

    UpdateConnectedClientListOnDelete(clSocket);

    // the return status will be the client # of this thread
    // timeToExit = iAmClient;
    // pthread_exit((void *)(timeToExit));
    printf("[SERVER (Thread-%02d)] : closing socket\n", iAmClient);

    return 0;
}

//
// Message List handler - this function is called (spawned as a thread of execution)
//

void *messageThread(void *dummy)
{
    while (exitMessageThread == 0)
    {
        // printf("\n--------------------Message thread executed Message count: %d\n\n", MessageQueueCount);
        if (MessageQueueCount > 0)
        {
            for (size_t posMesg = 0; posMesg < MessageQueueCount; posMesg++)
            {
                for (size_t posClient = 0; posClient < ConnectedClientsCount; posClient++)
                {
                    printf("Client Socket: %d, Message: %s \n", ConnectedClientsList[posClient].client_socket, MessageQueue[posMesg].message);
                    write(ConnectedClientsList[posClient].client_socket, MessageQueue[posMesg].message, strlen(MessageQueue[posMesg].message));
                }

                // check if message queue needs to be updated
                if (posMesg < MessageQueueCount - 1)
                {
                    for (size_t k = posMesg; k < ConnectedClientsCount - 1; k++)
                    {
                        MessageQueue[k] = MessageQueue[k + 1];
                    }
                }
            }
            MessageQueueCount--;
        }
        sleep(0.5f);
    }
}

void UpdateConnectedClientListOnDelete(int clSocket)
{
    for (size_t pos = 0; pos < ConnectedClientsCount; pos++)
    {
        if (ConnectedClientsList[pos].client_socket == clSocket)
        {
            // check if connected client list needs to be updated
            if (pos < ConnectedClientsCount - 1)
            {
                for (size_t k = pos; k < ConnectedClientsCount - 1; k++)
                {
                    ConnectedClientsList[k] = ConnectedClientsList[k + 1];
                }
            }
        }
    }

    // decrement the number of clients
    ConnectedClientsCount--;
}
