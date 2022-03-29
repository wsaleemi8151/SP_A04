/*
 * tcpip-server.c
 *
 * This is a sample internet server application that will respond
 * to requests on port 5000
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 5000

// used for accepting incoming command and also holding the command's response
char buffer[BUFSIZ];

// global to keep track of the number of connections
static int nClients = 0;
static int nNoConnections = 0;
  

/* Watch dog timer - to keep informed and watch how long the server goes without a connection */
void alarmHandler(int signal_number)
{
  if(nClients == 0) 
  {
     nNoConnections++;
     // It's been 10 seconds - determine how many 10 second intervals its been without a connection
     printf ("[SERVER WATCH-DOG] : It's been %d interval(s) without a new client connection ...\n", nNoConnections);
     alarm (10);	// reset alarm
  }
  else
  {
     // reset the number of times we've checked with no client connections
     nNoConnections = 0;
  }

  // reactivate signal handler for next time ...

  if((nNoConnections == 3) && (nClients == 0))
  {
     printf("[SERVER WATCH-DOG] : Its been 30 seconds without a new CLIENT ... LEAVING !\n");
     exit(-1);
  }
  signal (signal_number, alarmHandler);
}

/*
 * this signal handler is used to catch the termination
 * of the child. Needed so that we can avoid wasting
 * system resources when "zombie" processes are created
 * upon exit of the child (as the parent could concievably
 * wait for the child to exit)
 */

void WatchTheKids (int n)
{
    /* when this signal is invoked, it means that one of the children of
       this process (the clients) had exited.  So let's decrement the 
       nClients value here ... */
    nClients--;

    /* given that a CLIENT has exited, let's reset the alarm 
       to make us check in 10 seconds */
    alarm (10);	// reset alarm

    /* we received this signal because a CHILD (the CLIENT PROCESSOR) has exited ... 
       we (the SERVER) have no intention of doing anything or need anything from 
       that process - so the wait3() function call tells the O/S that we are 
       not waiting on anything from the terminated process ... so free up its 
       resources and get rid of it ...

       ... without this wait3() call ... the system would be littered with ZOMBIES */
    wait3 (NULL, WNOHANG, NULL);    
    signal (SIGCHLD, WatchTheKids);
}

int main (void)
{
  int                server_socket, client_socket;
  int                client_len;
  struct sockaddr_in client_addr, server_addr;
  int                len, i;
  FILE*              p;


  /*
   * install a signal handler for SIGCHILD (sent when the child terminates)
   */
  printf ("[SERVER] : Installing signal handler to manage ZOMBIE processes ...\n");
  signal (SIGCHLD, WatchTheKids);
  printf ("[SERVER] : Installing signal handler for WATCHDOG ...\n");
  signal (SIGALRM, alarmHandler);
  alarm (10);
  fflush(stdout);	// try to ensure that the print messages appear on the screen

  /*
   * obtain a socket for the server
   */
  printf ("[SERVER] : Obtaining STREAM Socket ...\n");
  fflush(stdout);
  if ((server_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
  {
    printf ("[SERVER] : Server Socket.getting - FAILED\n");
    return 1;
  }

  /*
   * initialize our server address info for binding purposes
   */
  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
  server_addr.sin_port = htons (PORT);

  printf ("[SERVER] : Binding socket to server address ...\n");
  fflush(stdout);
  if (bind (server_socket, (struct sockaddr *)&server_addr, sizeof (server_addr)) < 0) 
  {
    printf ("[SERVER] : Binding of Server Socket - FAILED\n");
    close (server_socket);
    return 2;
  }

  /*
   * start listening on the socket
   */
  printf ("[SERVER] : Begin listening on socket for incoming message ...\n");
  fflush(stdout);
  if (listen (server_socket, 5) < 0) 
  {
    printf ("[SERVER] : Listen on Socket - FAILED.\n");
    close (server_socket);
    return 3;
  }

  /*
   * for this server, run an endless loop that will
   * accept incoming requests from a remote client.
   * the server will fork a child copy of itself to handle the
   * request, and the parent will continue to listen for the
   * next request
   */
  while (1) 
  {
    /*
     * accept a packet from the client
     */
    client_len = sizeof (client_addr);
    if ((client_socket = accept (server_socket,(struct sockaddr *)&client_addr, &client_len)) < 0) 
    {
      printf ("[SERVER] : Accept Packet from Client - FAILED\n");
      close (server_socket);
      return 4;
    }

    /* Clear out the Buffer */
    memset(buffer,0,BUFSIZ);

	printf("[SERVER] : Incoming CLIENT connection on SOCKET %d\n", client_socket);
	fflush(stdout);
	nClients++;

    /*
     * fork a child to do the actual processing for the incoming command
     */
    if (fork() == 0) 
    {
	int whichClientAmI = 0;
	// I  am the child, so let's process the incoming client connection
	whichClientAmI = nClients;

	// we have an incoming connection - reset the global count to restart the "no client" countdown
	nNoConnections = 0;

	printf ("  [SERVER'S CHILD #%d] : Closing SERVER socket ...\n", whichClientAmI);
	fflush(stdout);
        close(server_socket);   // the CHILD doesn't need this ...

        /*
         * this is done by CHILD ONLY!
         *
         * read a block of info max BUFSIZE. compare 
         * against 3 commands: date, who, df
         */
        read (client_socket, buffer, BUFSIZ);
	printf ("  [SERVER'S CHILD #%d] : Processing command [%s] ...\n", whichClientAmI, buffer);
	fflush(stdout);

        while(strcmp(buffer,"quit") != 0)
        {
          /*
           * process command, and obtain outgoing data
           */
           if (strcmp (buffer, "date") == 0) 
           {
	      if ((p = popen ("date", "r")) != NULL) 
	      {
		  len = fread (buffer, 1, sizeof (buffer), p);
  		  pclose (p);
   	      } 
 	      else 
	      {
		strcpy (buffer, "Can't run [date] command\n");
		len = strlen (buffer);
	      } 
           } 
           else if (strcmp (buffer, "who") == 0) 
           {
	     if ((p = popen ("who", "r")) != NULL) 
	     {
	       len = fread (buffer, 1, sizeof (buffer), p);
	       pclose (p);
	     } 
	     else 
	     {
	       strcpy (buffer, "Can't run [who] command\n");
	       len = strlen (buffer);
	     }
           }  
           else if (strcmp (buffer, "df") == 0) 
           {
	     if ((p = popen ("df", "r")) != NULL) 
	     {
	       len = fread (buffer, 1, sizeof (buffer), p);
	       pclose (p);
	     } 
	     else 
 	     {
	       strcpy (buffer, "Can't run [df] command\n");
	       len = strlen (buffer);
	     }
           } 
           else 
           {
	     int   lowLevelFDForPipe;
             int   currFlags;
	     char  incomingCommand[BUFSIZ];

             // take a copy of the incoming command ... we may need it for a failure message */
             strcpy(incomingCommand, buffer);

	     if ((p = popen (buffer, "r")) != NULL) 
	     {
		/* let's check the returned data-length from the read - if it 
                   is 0 then assume that the command ended in a "Commond not found"
                   error and send that result */
	        len = fread (buffer, 1, sizeof (buffer), p);

                /* if we returned with no data from our popen() call -- 
                   then tell the user */
                if(len == 0)
                {
	          sprintf(buffer, "Can't run [%s] command\n", incomingCommand);
	          len = strlen (buffer);
                }
	        pclose (p);
	     } 
	     else 
 	     {
	       strcpy (buffer, "Can't run df command\n");
	       len = strlen (buffer);
	     }
           }	

           /*
            * write data to client, close socket, and exit child app
            */
	   printf ("  [SERVER'S CHILD #%d] : Sending response on CLIENT Socket ...\n", whichClientAmI);
	   fflush(stdout);
           write (client_socket, buffer, len);

	   // clear out and get the next command and process
           memset(buffer,0,BUFSIZ);
           read (client_socket, buffer, BUFSIZ);
	   printf ("  [SERVER'S CHILD #%d] : Processing command [%s] ...\n", whichClientAmI, buffer);
	   fflush(stdout);

	}

	printf ("  [SERVER'S CHILD #%d] : Closing CLIENT Socket and EXITING...\n", whichClientAmI);
	fflush(stdout);
        close (client_socket);

        return 0;	// this will signal the parent (server)
    } 
    else 
    {
      /*
       * this is done by parent ONLY - we'll close the client socket that we
       * opened and go back and listen for another command
       */
      printf("[SERVER] : Closing CLIENT Socket\n");
      fflush(stdout);
      close (client_socket);
    }
  }

  return 0;
}



