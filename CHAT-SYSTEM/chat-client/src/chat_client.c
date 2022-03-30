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
#include <string.h>

#include <ncurses.h>

#include "../inc/ncurser_helper.h"

int main(void)
{
    WINDOW *chat_win, *msg_win;
    int chat_startx, chat_starty, chat_width, chat_height;
    int msg_startx, msg_starty, msg_width, msg_height, i;
    int shouldBlank;
    char buf[BUFSIZ];

    initscr(); /* Start curses mode            */
    cbreak();
    noecho();
    refresh();

    shouldBlank = 0;

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

    /* allow the user to input 5 messages for display */
    for (i = 0; i < 5; i++)
    {
        input_win(chat_win, buf);
        display_win(msg_win, buf, i, shouldBlank);
    }
    sleep(3); /* to get a delay */

    /* tell the user that the 5 messages are done ... */
    shouldBlank = 1;
    sprintf(buf, "Messaging is complete ... destroying window in 5 seconds");
    display_win(msg_win, buf, 1, shouldBlank);

    sleep(1); /* to get a delay */

    destroy_win(chat_win);
    destroy_win(msg_win);
    endwin();
}
