/* ncurses help: Sam Hsu (11/17/10) */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

#include <ncurses.h>

#include "../inc/chat_client.h"
#include "../inc/ncurser_helper.h"
#include "../../Common/inc/common.h"

// Global variable for communication
static int my_server_socket;
static int display_window_row_count = 0;
WINDOW *chat_win, *output_win;
pthread_t tid[2]; // for holding message and chat windows logic threads
static int clientActive = 0;

static pthread_t inputWindowThreadId;
static pthread_t outputWindowThreadId;
char userId[USER_ID_LENGTH];

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    struct hostent *host;
    char buf[INPUT_MESG_LENGTH];

    char serverName[30];

    char serverNamePrefix[] = "-server";

    /*
     * check for sanity
     */
    if (argc != 3 || (argc == 3 && checkPrefix(userPrefix, argv[1]) != 0 && checkPrefix(serverNamePrefix, argv[2]) != 0))
    {
        printf("USAGE : chat-client -user<User> -server<Server Name/IP> \n");
        return 1;
    }

    // extracting command line arguments
    size_t userPrefixLen = strlen(userPrefix);
    size_t firstArgLen = strlen(argv[1]);

    size_t serverNamePrefixLen = strlen(serverNamePrefix);
    size_t secondArgLen = strlen(argv[2]);

    strncpy(userId, &argv[1][userPrefixLen], firstArgLen - userPrefixLen);
    strncpy(serverName, &argv[2][serverNamePrefixLen], secondArgLen - serverNamePrefixLen);

    /*
     * determine host info for server name supplied
     */
    if ((host = gethostbyname(serverName)) == NULL)
    {
        printf("[CHAT CLIENT] : Host Info Search - FAILED\n");
        return 2;
    }

    InitializeChatWindows();

    InitializeChatSocket(server_addr, host);

    clientActive = 1;
    blankWin(chat_win);

    int dummy;
    // Thread to dispatch received messages to all clients
    if (pthread_create(&inputWindowThreadId, NULL, inputWindowThread, (void *)&dummy))
    {
        sprintf(buf, "[CHAT CLIENT] : Input Window Thread FAILED\n");
        display_win(output_win, buf, 1, CLEAR_WINDOW);
        fflush(stdout);
        return 5;
    }

    // Thread to dispatch received messages to all clients
    if (pthread_create(&outputWindowThreadId, NULL, outputWindowThread, (void *)&dummy))
    {
        sprintf(buf, "[CHAT CLIENT] : Output Window Thread() FAILED\n");
        display_win(output_win, buf, 1, CLEAR_WINDOW);
        fflush(stdout);
        return 5;
    }

    int joinStatus = pthread_join(inputWindowThreadId, (void *)(&dummy));
    if (joinStatus == 0)
    {
        sprintf(buf, "\n[CHAT CLIENT] : Input Message thread completed");
        display_win(output_win, buf, 1, CLEAR_WINDOW);
    }

    sleep(0.5f);

    joinStatus = pthread_join(outputWindowThreadId, (void *)(&dummy));
    if (joinStatus == 0)
    {
        sprintf(buf, "\n[CHAT CLIENT] : Output Message thread completed");
        display_win(output_win, buf, 1, CLEAR_WINDOW);
    }

    sleep(0.5f);

    sprintf(buf, "[CHAT CLIENT] : I'm outta here !\n");
    display_win(output_win, buf, 1, CLEAR_WINDOW);

    // ----------------------------------------------------------------------------------------------

    /*
     * cleanup
     */
    close(my_server_socket);

    // ----------------------------------------------------------------------------------------------

    destroy_win(chat_win);
    destroy_win(output_win);
    endwin();

    return 1;
}

void InitializeChatWindows()
{
    int chat_startx, chat_starty, chat_width, chat_height;
    int msg_startx, msg_starty, msg_width, msg_height;

    initscr(); /* Start curses mode            */
    cbreak();
    noecho();
    refresh();

    chat_height = 5;
    chat_width = COLS - 2;
    chat_startx = 1;
    chat_starty = LINES - chat_height;

    msg_height = LINES - chat_height - 1;
    msg_width = COLS;
    msg_startx = 0;
    msg_starty = 0;

    initscr();
    refresh();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_GREEN);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);

    /* create the input window */
    output_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx);
    scrollok(output_win, TRUE);
    wbkgd(output_win, COLOR_PAIR(1));

    /* create the output window */
    chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx);
    scrollok(chat_win, TRUE);
    wbkgd(chat_win, COLOR_PAIR(2));
}

int InitializeChatSocket(struct sockaddr_in server_addr, struct hostent *host)
{
    // ---------------------------- Socket Implementation for Client --------------------------------
    char buf[INPUT_MESG_LENGTH];

    /*
     * initialize struct to get a socket to host
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length); // copy the host's internal IP addr into the server_addr struct
    server_addr.sin_port = htons(PORT);

    /*
     * get a socket for communications
     */
    sprintf(buf, "[CHAT CLIENT] : Getting STREAM Socket to talk to SERVER\n");
    display_win(output_win, buf, 1, CLEAR_WINDOW);
    sleep(0.5f);

    fflush(stdout);
    if ((my_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(buf, "[CLIENT] : Getting Client Socket - FAILED\n");
        display_win(output_win, buf, 1, CLEAR_WINDOW);

        destroy_win(chat_win);
        destroy_win(output_win);
        endwin();

        return 3;
    }

    /*
     * attempt a connection to server
     */
    sprintf(buf, "[CLIENT] : Connecting to SERVER\n");
    display_win(output_win, buf, 1, CLEAR_WINDOW);
    sleep(0.5);

    fflush(stdout);
    if (connect(my_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        sprintf(buf, "[CLIENT] : Connect to Server - FAILED\n");
        display_win(output_win, buf, 1, CLEAR_WINDOW);
        close(my_server_socket);

        destroy_win(chat_win);
        destroy_win(output_win);
        endwin();

        return 4;
    }
    else
    {
        // send user id as first message
        /* clear out the contents of buffer (if any) */
        char uId[USER_ID_LENGTH];
        memset(uId, 0, USER_ID_LENGTH);
        sprintf(uId, "-user%s", userId);
        write(my_server_socket, uId, strlen(uId));
    }
}

//
// Input Winow handler - this function is called (spawned as a thread of execution)
//

void *inputWindowThread(void *dummy)
{
    char buf[INPUT_MESG_LENGTH];

    display_window_row_count = 1;
    while (clientActive)
    {
        /* clear out the contents of buffer (if any) */
        memset(buf, 0, INPUT_MESG_LENGTH);

        /*
         * now that we have a connection, get a commandline from
         * the user, and fire it off to the server
         */
        fflush(stdout);

        input_win(chat_win, buf);

        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';

        /* check if the user wants to quit */
        if (strcmp(buf, "quit") == 0)
        {
            // send the command to the SERVER
            write(my_server_socket, buf, strlen(buf));
            clientActive = 0;
        }
        else
        {
            write(my_server_socket, buf, strlen(buf));
        }
    }
}

//
// Output Winow handler - this function is called (spawned as a thread of execution)
//

void *outputWindowThread(void *dummy)
{
    char buf[INPUT_MESG_LENGTH];

    display_window_row_count = 1;
    while (clientActive)
    {
        /* clear out the contents of buffer (if any) */
        memset(buf, 0, INPUT_MESG_LENGTH);

        /*
         * now that we have a connection, get a commandline from
         * the user, and fire it off to the server
         */
        fflush(stdout);

        read(my_server_socket, buf, sizeof(buf));

        display_win(output_win, buf, display_window_row_count, NOT_CLEAR_WINDOW);
        display_window_row_count++;
    }
}