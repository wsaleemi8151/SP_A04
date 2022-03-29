/*
 * tcpip-client.c
 *
 * This is a sample internet client application that will talk
 * to the server s.c via port 5000
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

#define PORT 5000

char buffer[BUFSIZ];

int main (int argc, char *argv[])
{
  int                my_server_socket, len, done;
  int                whichClient;
  struct sockaddr_in server_addr;
  struct hostent*    host;

  /*
   * check for sanity
   */
  if (argc != 3) 
  {
    printf ("USAGE : tcpipClient <clientID> <server_name>\n");
    return 1;
  }

  whichClient = atoi(argv[1]);

  /*
   * determine host info for server name supplied
   */
  if ((host = gethostbyname (argv[2])) == NULL) 
  {
    printf ("[CLIENT-%d] : Host Info Search - FAILED\n",whichClient);
    return 2;
  }

  /*
   * initialize struct to get a socket to host
   */
  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  memcpy (&server_addr.sin_addr, host->h_addr, host->h_length); // copy the host's internal IP addr into the server_addr struct
  server_addr.sin_port = htons (PORT);

     /*
      * get a socket for communications
      */
     printf ("[CLIENT-%d] : Getting STREAM Socket to talk to SERVER\n", whichClient);
     fflush(stdout);
     if ((my_server_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
     {
       printf ("[CLIENT-%d] : Getting Client Socket - FAILED\n", whichClient);
       return 3;
     }

  
     /*
      * attempt a connection to server
      */
     printf ("[CLIENT-%d] : Connecting to SERVER\n", whichClient);
     fflush(stdout);
     if (connect (my_server_socket, (struct sockaddr *)&server_addr,sizeof (server_addr)) < 0) 
     {
       printf ("[CLIENT-%d] : Connect to Server - FAILED\n", whichClient);
       close (my_server_socket);
       return 4;
     }

  done = 1;
  memset(buffer,0,BUFSIZ);
  if(whichClient == 1) 	printf ("Enter your [<chat text> | quit]");

  if(whichClient == 2)
  {
	len = read (my_server_socket, buffer, BUFSIZ);
	printf ("<< %s\n", buffer);
	memset(buffer,0,BUFSIZ);
        fflush(stdout);
  }

  while(done)
  {
     /* clear out the contents of buffer (if any) */
     memset(buffer,0,BUFSIZ);

     /*
      * now that we have a connection, get a commandline from
      * the user, and fire it off to the server
      */
     if(whichClient == 1) 
     {
	memset(buffer,0,BUFSIZ);
	printf (">> ");
	fflush (stdout);
	fgets (buffer, BUFSIZ, stdin);
	if (buffer[strlen (buffer) - 1] == '\n') buffer[strlen (buffer) - 1] = '\0';

	/* check if the user wants to quit */
	if(strcmp(buffer,"quit") == 0)
	{
		// send the command to the SERVER
		fflush (stdout);
		write (my_server_socket, buffer, strlen (buffer));
		done = 0;
	}
	else
	{
		write (my_server_socket, buffer, strlen (buffer));
		memset(buffer,0,BUFSIZ);
		len = read (my_server_socket, buffer, BUFSIZ);
		printf ("<< %s\n", buffer);
		fflush (stdout);
	}
     }
     else
     {
	memset(buffer,0,BUFSIZ);
	printf (">> ");
	fflush (stdout);
	fgets (buffer, BUFSIZ, stdin);
	if (buffer[strlen (buffer) - 1] == '\n') buffer[strlen (buffer) - 1] = '\0';

	/* check if the user wants to quit */
	if(strcmp(buffer,"quit") == 0)
	{
		// send the command to the SERVER
		fflush (stdout);
		write (my_server_socket, buffer, strlen (buffer));
		done = 0;
	}
	else
	{
		write (my_server_socket, buffer, strlen (buffer));
		memset(buffer,0,BUFSIZ);
		len = read (my_server_socket, buffer, BUFSIZ);
		printf ("<< %s\n", buffer);
		fflush (stdout);
	}
     }
  }

     /*
      * cleanup
      */
     close (my_server_socket);

  printf ("[CLIENT-%d] : I'm outta here !\n", whichClient);

  return 0;
}



