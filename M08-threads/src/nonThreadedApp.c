/*
 * nonThreadedApp.c
 *
 * This program demonstrates a non-threaded version of a program.
 * This program performs exactly the same functionality
 * as the threaded application - but processes each word sequentially 
 * as a single threaded application would ...
 */


#define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


/*
 * prototypes
 */

void* my_thread_function (void *);
int   really_waste_some_time (int);


int main (int argc, char *argv[])
{
  int*          pStatus;
  int		x;
  time_t	startTime, stopTime;
  
  /* timestamp the start of the application */
  startTime = time(NULL);

  /*
   * only run if 2 or more arguments available and limit
   * number of commandline arguments to 10 additional arguments
   */
  if (argc < 2) 
  {
    printf ("Usage: nonThreadedApp word1 word2 word3 ...\n");
    exit (1);
  }

  if (argc > 11) argc = 11;

  /*
   * run function for each argument - which is the same as the "threaded" version of the program
   * to linearize the processing
   */
  for (x = 0; x < (argc - 1); x++) 
  {
    pStatus = (int *)my_thread_function ((void *)argv[x + 1]);
    printf ("Thread ID: %u length of argument %d is %d\n", (unsigned int)pthread_self(), (x+1), *pStatus);

    // remember to free the memory we allocated inside the thread
    free(pStatus);
  }

  /*
   * we're done!
   */
  printf ("All threads done! Thanx for coming ...\n");

  /* timestamp the stop of the application */
  stopTime = time(NULL);
  printf("*** TIME SPENT : %d seconds\n", (int)(stopTime-startTime));

}


/*
 * void *my_thread_function (void *data);
 *
 * This is a thread function. it accepts a single pointer
 * to any arbitrary data from the caller, and returns a pointer
 * to arbitrary data. Note the use of the function pthread_self() - 
 * it will obtain the thread ID of the current thread, to help
 * keep the threads apart visually during their execution.
 */

void * my_thread_function (void *data)
{
  char*     word     = (char *)data;
  int       lenOfWord = strlen(word);
  int*      retValue;
  pthread_t myID = pthread_self();
  int x;

  // we need to allocate memory for the return value here because we have made the return data-type
  // the same as the required one in the threaded example
  retValue = malloc(sizeof(int));  // allocate some space for the return value	

  /*
   * simulate some work by performing some goofy
   * array operations on the incoming string - spell the word out character by character
   */
  printf ("Thread ID: %u forwards: ", (unsigned int)myID);
  for (x = 0; x < lenOfWord; x++)
  {
    printf ("%c", word[x]);
  }
  printf ("\n");
  fflush (stdout);

  /* now have a "sleep" for x seconds where x is the # of characters in the word */	
  really_waste_some_time (lenOfWord);

  /* now spell the word out backwards - one character at a time */
  printf ("Thread ID: %u backwards: ", (unsigned int)myID);
  for (x = 0; x < lenOfWord; x++)
  {
    printf ("%c", word[lenOfWord - 1 - x]);
  }
  printf ("\n");
  fflush (stdout);

  /* again have a sleep */	
  really_waste_some_time (lenOfWord);

  /* now print out the word in UPPERCASE - one character at a time */
  printf ("Thread ID: %u uppercase: ", (unsigned int)myID);
  for (x = 0; x < lenOfWord; x++)
  {
    printf ("%c", toupper(word[x]));
  }
  printf ("\n");
  fflush (stdout);

  /* snooze time */	
  really_waste_some_time (lenOfWord);

  /* finally spell the word all in lowercase */
  printf ("Thread ID: %u lowercase: ", (unsigned int)myID);
  for (x = 0; x < lenOfWord; x++)
  {
    printf ("%c", tolower(word[x]));
  }
  printf ("\n");
  fflush (stdout);

  /* one final sleep */	
  really_waste_some_time (lenOfWord);
 
  /*
   * enough simulated work. let's return a status value
   * now ...
   */
  printf ("Thread ID: %u COMPLETE!\n", (unsigned int)myID);

  // if we are going to detach (and not join) the threads back in the main thread,
  // then comment out the following 2 lines and uncomment the third.
  *retValue=lenOfWord;	
  return ((void*)retValue);
}


/*
 * int really_waste_some_time (int t);
 *
 * This function just sleeps for the sake of wasting
 * some time. It's meant to simulate some "real" work
 * happening.
 */

int really_waste_some_time (int t)
{
  sleep (t);
  return 0;
}
















