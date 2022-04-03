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
static int exitCalled = 0;
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
    int onStartup = 1;

    /*
     * obtain a socket for the server
     */

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("[CHAT SERVER] : socket() FAILED\n");
        return 1;
    }
    printf("[CHAT SERVER] : socket() successful\n");

    /*
     * initialize our server address info for binding purposes
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[CHAT SERVER] : bind() FAILED\n");
        close(server_socket);
        return 2;
    }
    printf("[CHAT SERVER] : bind() successful\n");

    /*
     * start listening on the socket
     */
    if (listen(server_socket, 5) < 0)
    {
        printf("[CHAT SERVER] : listen() - FAILED.\n");
        close(server_socket);
        return 3;
    }
    printf("[CHAT SERVER] : listen() successful\n");

    // Thread to dispatch received messages to all clients
    if (pthread_create(&messageThreadId, NULL, messageThread, (void *)&client_socket))
    {
        printf("[CHAT SERVER] : Message Thread execution FAILED\n");
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
    while (exitCalled == 0)
    {
        if (ConnectedClientsCount >= NO_OF_CLIENTS)
        {
            exitCalled = 1;
            printf("[CHAT SERVER] : Max number of clients connected, not accepting new clients\n");
        }
        else if (ConnectedClientsCount == 0 && onStartup == 0)
        {
            exitCalled = 1;
        }
        else
        {
            printf("[CHAT SERVER] : Ready to accept()\n");
            fflush(stdout);

            /*
             * accept a packet from the client if not max number of clients reached
             */
            client_len = sizeof(client_addr);
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0)
            {
                printf("[CHAT SERVER] : accept() FAILED\n");
                fflush(stdout);
                return 4;
            }

            // On Startup set to false when atleast one client successfully connected
            onStartup = 1;

            fflush(stdout);

            // reading user id
            char userId[USER_ID_LENGTH];

            char buffer[INPUT_MESG_LENGTH];

            // clear out and get the next command and process
            memset(buffer, 0, INPUT_MESG_LENGTH);
            int numBytesRead = read(client_socket, buffer, INPUT_MESG_LENGTH);
            if (checkPrefix(userPrefix, buffer) == 0)
            {
                size_t firstMesgLen = strlen(buffer);
                size_t userPrefixLen = strlen(userPrefix);

                size_t substringLen = 0;
                if (firstMesgLen > userPrefixLen + 5)
                {
                    substringLen = userPrefixLen + 5;
                }
                else
                {
                    substringLen = firstMesgLen;
                }
                strncpy(userId, &buffer[userPrefixLen], substringLen);
            }
            else
            {
                printf("[CHAT SERVER] : Invalid User Id\n");
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

            memset(clientSocketSt.userId, 0, USER_ID_LENGTH);
            strcpy(clientSocketSt.userId, userId);

            if (pthread_create(&(ConnectedClientsList[ConnectedClientsCount].tid), NULL, socketThread, (void *)&clientSocketSt))
            {
                printf("[CHAT SERVER] : Socket Thread execution FAILED\n");
                fflush(stdout);
                return 5;
            }
            else
            {
                ConnectedClientsList[ConnectedClientsCount].client_socket = client_socket;
                ConnectedClientsList[ConnectedClientsCount].client_addr = &client_addr;
                strcpy(ConnectedClientsList[ConnectedClientsCount].userId, userId);
                ConnectedClientsCount++;
            }

            fflush(stdout);
        }
    }

    fflush(stdout);

    for (i = 0; i < NO_OF_CLIENTS; i++)
    {
        int joinStatus = pthread_join(ConnectedClientsList[i].tid, (void *)(&whichClient));
        if (joinStatus == 0)
        {
            printf("\n[CHAT SERVER] : Received QUIT command from CLIENT-%02d (joinStatus=%d)\n", whichClient, joinStatus);
        }
    }

    // exiting message thread
    int joinStatus = pthread_join(messageThreadId, (void *)(&whichClient));
    if (joinStatus == 0)
    {
        printf("\n[CHAT SERVER] : Message thread completed");
    }

    printf("\n[CHAT SERVER] : All clients have returned - exiting ...\n");
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

    // increment the numClients
    int iAmClient = ConnectedClientsCount; // assumes that another connection from another client
                                           // hasn't been created in the meantime

    /* Clear out the input Buffer */
    memset(message, 0, INPUT_MESG_LENGTH);
    memset(buffer, 0, INPUT_MESG_LENGTH);

    numBytesRead = read(clSocket, buffer, INPUT_MESG_LENGTH);

    while (strcmp(buffer, "quit") != 0)
    {
        sprintf(message, "%s", buffer);

        // storing received message in the message queue to be broadcast
        if (MessageQueueCount < MESSAGE_QUEUE_LENGTH && strlen(message) > 0)
        {
            // Update message queue when a message received
            MessageQueue[MessageQueueCount].client_socket = clSocket;

            if (strlen(message) > SINGLE_MESG_MAX_LENGTH)
            {
                // if no space found in between 35 to 40, then split at position 40
                int messageSplitter = SINGLE_MESG_MAX_LENGTH;
                for (size_t i = 35; i < SINGLE_MESG_MAX_LENGTH + 1; i++)
                {
                    // if space found
                    if ((int)message[i] == 32)
                    {
                        messageSplitter = i;
                        break;
                    }
                }

                char firstMesg[SINGLE_MESG_MAX_LENGTH];
                strncpy(firstMesg, &message[0], messageSplitter);
                char secondMesg[SINGLE_MESG_MAX_LENGTH];
                strncpy(secondMesg, &message[messageSplitter], strlen(message) - messageSplitter);

                strcpy(MessageQueue[MessageQueueCount].message, firstMesg);
                strcpy(MessageQueue[MessageQueueCount].userId, clientSocketSt.userId);
                MessageQueueCount++;

                strcpy(MessageQueue[MessageQueueCount].message, secondMesg);
                strcpy(MessageQueue[MessageQueueCount].userId, clientSocketSt.userId);
                MessageQueueCount++;
            }
            else
            {
                strcpy(MessageQueue[MessageQueueCount].message, message);
                strcpy(MessageQueue[MessageQueueCount].userId, clientSocketSt.userId);
                MessageQueueCount++;
            }
        }

        // clear out and get the next command and process
        memset(buffer, 0, INPUT_MESG_LENGTH);
        numBytesRead = read(clSocket, buffer, INPUT_MESG_LENGTH);
    }
    close(clSocket);

    UpdateConnectedClientListOnDelete(clSocket);

    // the return status will be the client # of this thread
    printf("[CHAT SERVER] : (Thread-%02d) closing socket\n", iAmClient);
    timeToExit = iAmClient;
    pthread_exit((void *)(&timeToExit));
}

//
// Message List handler - this function is called (spawned as a thread of execution)
//

void *messageThread(void *dummy)
{
    while (exitCalled == 0)
    {
        if (MessageQueueCount > 0)
        {
            for (size_t posMesg = 0; posMesg < MessageQueueCount; posMesg++)
            {
                for (size_t posClient = 0; posClient < ConnectedClientsCount; posClient++)
                {
                    // Help retreived from: How to get ip address from sock structure in c?
                    // https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c

                    struct sockaddr_in *pV4Addr = (struct sockaddr_in *)&ConnectedClientsList[posClient].client_addr;
                    struct in_addr ipAddr = ConnectedClientsList[posClient].client_addr->sin_addr;
                    char str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);

                    char buffer[INPUT_MESG_LENGTH];

                    /* clear out the contents of buffer (if any) */
                    memset(buffer, 0, INPUT_MESG_LENGTH);

                    if (ConnectedClientsList[posClient].client_socket == MessageQueue[posMesg].client_socket)
                    {
                        sprintf(buffer, "%-16s [%-5s] >> %s", str, MessageQueue[posMesg].userId, MessageQueue[posMesg].message);
                    }
                    else
                    {
                        sprintf(buffer, "%-16s [%-5s] << %s", str, MessageQueue[posMesg].userId, MessageQueue[posMesg].message);
                    }

                    write(ConnectedClientsList[posClient].client_socket, buffer, strlen(buffer));
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
