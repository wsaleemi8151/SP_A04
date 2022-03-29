/*
 * threadedApp.c
 *
 * This program demonstrates threading an application in order to increase
 * the perceived/actual throughput of the program
 */


#define _REENTRANT

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/*
 * prototypes
 */

void* my_thread_function (void *);
int   really_waste_some_time (int);

// the following variable is used to control the flow of the main thread (i.e. main()) if we detach() the
// threads instead of joining() them ... notice this is a GLOBAL variable, but by design - I have deemed it
// safe to use
static int globalCounter;		

int main (int argc, char *argv[])
{
  int*          ptr_status;
  int		x;
  time_t	startTime, stopTime;
  pthread_t 	tid[10];

  /* timestamp the start of the application */
  startTime = time(NULL);

  globalCounter = 0;

  /*
   * only run if 2 or more arguments available and limit
   * number of commandline arguments to 10 additional arguments
   */
  if (argc < 2) 
    {
      printf ("usage: threadedApp word1 word2 word3 ...\n");
      exit (1);
    }

  if (argc > 11) argc = 11;

  /*
   * run a loop for each additional argument on commandline
   * and start a thread of execution, giving each thread the
   * appropriate commandline argument. the thread will 
   * "play" with the word and rearrange the letters in the word
   * to simulate real work. Note how we keep track of each
   * thread's ID in the tid array.
   *
   * Basically - for each word on the command line a thread is
   * created that will perform some processing on the word.  Once all
   * word processing threads are complete - they will join up and the
   * program will continue as a single processing thread
   */
  for (x = 0; x < (argc - 1); x++) 
  {
    if (pthread_create (&tid[x], NULL, my_thread_function, (void *)argv[x + 1]) != 0) 
    {
      printf ("Arghh !! I cannot start thread #%d\n", x);
      exit (2);
    }
  }

  /*
   * wait for each thread to finish up, and the return
   * status of each thread will be the length of each
   * corresponding argument on the commandline. Note that
   * the ptr_status variable is used to communicate the location
   * of the status variable for the pthread_join function to fill
   * in on our behalf.
   */
  for (x = 0; x < (argc - 1); x++) 
  {
     // we can choose to wait and "join" all of the threads or detach the threads and allow the main
     // thread to continue running as the threads run in the background
     //
     // By default I will join them in the order they were launched ... it makes sense to me that 
     // Thread #1 should finish before Thread #2 but as we know with task (and now thread) scheduling
     // - there is no certainty

     // JOINING the threads - if you are going to detach, then comment out the following 3 lines
     pthread_join (tid[x],(void**)&ptr_status);
     printf ("Thread ID: %u length of argument %d is %d\n", (unsigned int)tid[x],(x+1),*ptr_status);
       // remember to free the memory we allocated inside the thread
       free(ptr_status);

     // DETACHING THREADS - if you detach the threads then make sure to uncomment the "while(globalCounter)..."
     // line below as well ...
//	pthread_detach (tid[x]);

  }

//  while(globalCounter>0);

  /*
   * we're done!
   */
  printf ("All threads done! Thanks for coming ...\n");

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
 *
 */

void* my_thread_function (void *data)
{
  char*     word     = (char *)data;
  int       lenOfWord = strlen(word);
  int*      retValue;
  pthread_t myID = pthread_self();
  int x;

  globalCounter++;

  // if we are going to detach (and not join) the threads back in the main thread,
  // then comment out the following line.
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
    printf ("%c", toupper (word[x]));
  }
  printf ("\n");
  fflush (stdout);

  /* snooze time */	
  really_waste_some_time (lenOfWord);

  /* finally spell the word all in lowercase */
  printf ("Thread ID: %u lowercase: ", (unsigned int)myID);
  for (x = 0; x < lenOfWord; x++)
  {
    printf ("%c", tolower (word[x]));
  }
  printf ("\n");
  fflush (stdout);

  /* one final sleep */	
  really_waste_some_time (lenOfWord);
 
  /*
   * enough simulated work. let's return a status value
   * now ...
   */
  globalCounter--;
  printf ("Thread ID: %u COMPLETE! Thread count=%d\n", (unsigned int)myID, globalCounter);

  // if we are going to detach (and not join) the threads back in the main thread,
  // then comment out the following 2 lines and uncomment the third.
  *retValue=lenOfWord;	
  return ((void*)retValue);
//  return(NULL);
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


