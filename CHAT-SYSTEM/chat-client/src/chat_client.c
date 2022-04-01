/* splitWin.c is a simple example to show how to deal with split screens.
   Due to the limited time, this program is not finished yet.

   To compile:   gcc splitWin.c -lncurses

        Sam Hsu (11/17/10)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ncurses.h>

#include "../inc/chat_client.h"
#include "../inc/ncurser_helper.h"
#include "../../Common/inc/common.h"

// Global variable for communication
static int my_server_socket;
static int display_window_row_count = 0 ;
WINDOW *chat_win, *msg_win;
pthread_t tid[2]; // for holding message and chat windows logic threads
static int clientDone = 0;

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    struct hostent *host;

    /*
     * check for sanity
     */
    if (argc != 2)
    {
        printf("USAGE : tcpipClient <server_name>\n");
        return 1;
    }

    /*
     * determine host info for server name supplied
     */
    if ((host = gethostbyname(argv[1])) == NULL)
    {
        printf("[CLIENT] : Host Info Search - FAILED\n");
        return 2;
    }

    InitChatClient(server_addr, host);
    
    return 1;
}

int InitChatClient(struct sockaddr_in server_addr, struct hostent *host)
{
    char buf[INPUT_MESG_LENGTH];
    
    InitializeChatWindows(buf);

    InitializeChatSocket(server_addr, host, buf);


    clientDone = 1;
    blankWin(msg_win);

    display_window_row_count = 1;
    while (clientDone)
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
            clientDone = 0;
        }
        else
        {
            write(my_server_socket, buf, strlen(buf));
            read(my_server_socket, buf, sizeof(buf));

            display_win(msg_win, buf, display_window_row_count, NOT_CLEAR_WINDOW);
            display_window_row_count++;
        }
    }

    /*
     * cleanup
     */
    close(my_server_socket);

    sprintf(buf, "[CLIENT] : I'm outta here !\n");
    display_win(msg_win, buf, 1, CLEAR_WINDOW);

    // ----------------------------------------------------------------------------------------------

    destroy_win(chat_win);
    destroy_win(msg_win);
    endwin();

    return 1;
}

void InitializeChatWindows(char * buf)
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
    msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx);
    scrollok(msg_win, TRUE);
    wbkgd(msg_win, COLOR_PAIR(1));

    /* create the output window */
    chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx);
    scrollok(chat_win, TRUE);
    wbkgd(chat_win, COLOR_PAIR(2));

}

int InitializeChatSocket(struct sockaddr_in server_addr, struct hostent *host, char * buf)
{
    // ---------------------------- Socket Implementation for Client --------------------------------

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
    sprintf(buf, "[CLIENT] : Getting STREAM Socket to talk to SERVER\n");
    display_win(msg_win, buf, 1, CLEAR_WINDOW);
    sleep(1);

    fflush(stdout);
    if ((my_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(buf, "[CLIENT] : Getting Client Socket - FAILED\n");
        display_win(msg_win, buf, 1, CLEAR_WINDOW);

        sleep(1);

        destroy_win(chat_win);
        destroy_win(msg_win);
        endwin();

        return 3;
    }

    /*
     * attempt a connection to server
     */
    sprintf(buf, "[CLIENT] : Connecting to SERVER\n");
    display_win(msg_win, buf, 1, CLEAR_WINDOW);
    sleep(1);

    fflush(stdout);
    if (connect(my_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        sprintf(buf, "[CLIENT] : Connect to Server - FAILED\n");
        display_win(msg_win, buf, 1, CLEAR_WINDOW);
        close(my_server_socket);

        sleep(1);

        destroy_win(chat_win);
        destroy_win(msg_win);
        endwin();

        return 4;
    }

}