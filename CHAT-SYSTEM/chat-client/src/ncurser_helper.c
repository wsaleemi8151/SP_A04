#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ncurses.h>

#include "../../Common/inc/common.h"
#include "../inc/ncurser_helper.h"

WINDOW *create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);   /* draw a box */
    wmove(local_win, 1, 1); /* position cursor at top */
    wrefresh(local_win);
    return local_win;
}

/* This function is for taking input chars from the user */
void input_win(WINDOW *win, char *word)
{
    int i, ch;
    int maxrow, maxcol, row = 1, col = 0;

    blankWin(win); /* make it a clean window */

    mvwaddstr(win, 0, (COLS / 2 - 10), " Chat Window ");

    getmaxyx(win, maxrow, maxcol); /* get window size */

    bzero(word, INPUT_MESG_LENGTH);
    wmove(win, 1, 1); /* position cusor at top */
    for (i = 0; (ch = wgetch(win)) != '\n'; i++)
    {
        if (strlen(word) < INPUT_MESG_LENGTH - 1)
        {

            word[i] = ch;           /* '\n' not copied */
            if (col++ < maxcol - 2) /* if within window */
            {
                wprintw(win, "%c", word[i]); /* display the char recv'd */
            }
            else /* last char pos reached */
            {
                col = 1;
                if (row == maxrow - 2) /* last line in the window */
                {
                    scroll(win);          /* go up one line */
                    row = maxrow - 2;     /* stay at the last line */
                    wmove(win, row, col); /* move cursor to the beginning */
                    wclrtoeol(win);       /* clear from cursor to eol */
                    box(win, 0, 0);       /* draw the box again */
                }
                else
                {
                    row++;
                    wmove(win, row, col); /* move cursor to the beginning */
                    wrefresh(win);
                    wprintw(win, "%c", word[i]); /* display the char recv'd */
                }
            }
        }
    }
} /* input_win */

void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{

    if (shouldBlank == 1)
        blankWin(win); /* make it a clean window */
    mvwaddstr(win, 0, (COLS / 2 - 7), " Message Window ");

    wmove(win, (whichRow + 1), 1); /* position cusor at approp row */
    wprintw(win, word);
    wrefresh(win);
} /* display_win */

void destroy_win(WINDOW *win)
{
    delwin(win);
} /* destory_win */

void blankWin(WINDOW *win)
{
    int i;
    int maxrow, maxcol;

    getmaxyx(win, maxrow, maxcol);
    for (i = 1; i < maxcol - 2; i++)
    {
        wmove(win, i, 1);
        refresh();
        wclrtoeol(win);
        wrefresh(win);
    }
    box(win, 0, 0); /* draw the box again */

    wrefresh(win);
} /* blankWin */
