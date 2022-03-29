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
     printf ("[SERVER WATCH-DOG] : It's been %d interval(s) without any client connections or chatter ...\n", nNoConnections);
  }
  else
  {
     // reset the number of times we've checked with no client connections
     nNoConnections = 0;
  }

  // reactivate signal handler for next time ...

  if((nNoConnections == 3) && (nClients == 0))
  {
     printf("[SERVER WATCH-DOG] : Its been 30 seconds of inactivity ... I'M LEAVING !\n");
     exit(-1);
  }
  signal (signal_number, alarmHandler);
  alarm (10);	// reset alarm
}

int main (void)
{
  int                server_socket, client_socket1, client_socket2;
  int                client_len1, client_len2;
  int                client1Gone, client2Gone;
  struct sockaddr_in client_addr1, client_addr2, server_addr;
  int                len, i;
  FILE*              p;


  /*
   * install a signal handler for SIGCHILD (sent when the child terminates)
   */
  printf ("[SERVER] : Installing signal handler for WATCHDOG ...\n");
  signal (SIGALRM, alarmHandler);
  alarm (10);
  fflush(stdout);

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
	printf ("[SERVER] : Begin listening on socket for incoming connections ...\n");
	fflush(stdout);
  if (listen (server_socket, 5) < 0) 
  {
    printf ("[SERVER] : Listen on Socket - FAILED.\n");
    close (server_socket);
    return 3;
  }

  /*
   * this is a really crappy CHAT program using the socket API
   *   -- basically the server waits for 2 client connections and
   *      then does a back and forth chatting exchange.  In reality
   *      to manage the conversation better, some sharedMemory scheme, etc
   *      would need to be implemented between the client child processes
   */
  while (1) 
  {
    printf("[SERVER] : Now open for connections ... I need 2 chat clients ...\n");
    fflush(stdout);

    /*
     * accept a packet from the client1
     */
    client_len1 = sizeof (client_addr1);
    if ((client_socket1 = accept (server_socket,(struct sockaddr *)&client_addr1, &client_len1)) < 0) 
    {
      printf ("[SERVER] : Accept Packet from Client - FAILED\n");
      close (server_socket);
      return 4;
    }
    printf("[SERVER] : One client has 'signed up' ... waiting for a second client ... (%d)\n", client_socket1);
    client1Gone = 0;
    fflush(stdout);
    nClients++;

    /*
     * accept a packet from the client2
     */
    client_len2 = sizeof (client_addr2);
    if ((client_socket2 = accept (server_socket,(struct sockaddr *)&client_addr2, &client_len2)) < 0) 
    {
      printf ("[SERVER] : Accept Packet from Client - FAILED\n");
      close (server_socket);
      return 4;
    }
    client2Gone = 0;
    printf("[SERVER] : Okay, I now have 2 clients ... I feel like a radio talk-show host. (%d)\n", client_socket2);
    printf("           Client-1, you're on the air - what would you like to say ...\n");
    fflush(stdout);
    nClients++;

    /* Clear out the Buffer */
    memset(buffer,0,BUFSIZ);

    /*
     * implement the chat in brute force - no forking / no IPC to share the processor
     */
    while((client1Gone == 0) || (client2Gone==0))  // so long as someone is talking
    {
	if(client1Gone == 0) read (client_socket1, buffer, BUFSIZ);
        if(client1Gone == 0)
        {
		if((strcmp(buffer,"quit") == 0) || (strcmp(buffer,"QUIT") == 0))
		{
			memset(buffer,0,BUFSIZ);
			printf ("  [SERVER] : Client-1 is signing off ... Goodbye Client-1 !\n");
		        sprintf (buffer, "I'm done talking with you ... Goodbye!");
			nClients--;
	                client1Gone = 1;
			close (client_socket1);
			fflush(stdout);
		}
		else
		{
			if(client2Gone == 0)
                        {
                          printf ("  [SERVER] : Client-1, you make a good point.  I'll relay this information\n");
		          printf ("             to Client-2 and see what they have to say ...\n");
                        }
                        else
                        {
                           printf ("  [SERVER] : Man !! Is this guy still talking ? Client-2 is gone!!\n");
                        }
			fflush(stdout);
	    	}
	}
        else
        {
		sprintf (buffer, "[SERVER] Client-1 is gone ... let it go !!");
        }

	len = strlen (buffer);
    	if(client2Gone == 0) write (client_socket2, buffer, len);
	
	// clear out and get the next command and process
	memset(buffer,0,BUFSIZ);
	if(client2Gone == 0) read (client_socket2, buffer, BUFSIZ);
        if(client2Gone == 0)
        {
		if((strcmp(buffer,"quit") == 0) || (strcmp(buffer,"QUIT") == 0))
		{
  			memset(buffer,0,BUFSIZ);
			printf ("  [SERVER] : Client-2 is signing off ... Goodbye Client-2 !\n");
		        sprintf (buffer, "I'm done talking with you ... Goodbye!");
			nClients--;
	                client2Gone = 1;
			close (client_socket2);
			fflush(stdout);
		}
		else
		{
			if(client1Gone == 0)
                        {
                           printf ("  [SERVER] : Client-2, excellent counter-point.  Back to you Client-1\n");
                        }
                        else
                        {
                           printf ("  [SERVER] : Man !! Is this guy still talking ? Client-1 is gone!!\n");
                        }
			fflush(stdout);
	    	}
	}
        else
        {
		sprintf (buffer, "[SERVER] Client-2 is gone ... let it go !!");
        }

	
	len = strlen (buffer);
	if(client1Gone == 0) write (client_socket1, buffer, len);
	memset(buffer,0,BUFSIZ);
    }

    printf ("[SERVER] : Everyone is gone ... I'm leaving as well ...\n");
    fflush(stdout);
    close (server_socket);
    return 99;
  }

  return 0;
}



