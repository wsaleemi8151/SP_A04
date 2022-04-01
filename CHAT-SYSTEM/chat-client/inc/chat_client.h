
#define PORT 5000
#define CLEAR_WINDOW 1
#define NOT_CLEAR_WINDOW 0

int checkPrefix(char *pre, char *str);
int InitChatClient(struct sockaddr_in server_addr, struct hostent *host);
void InitializeChatWindows(char * buf);
int InitializeChatSocket(struct sockaddr_in server_addr, struct hostent *host, char * buf);

void *inputWindowThread(void *);
void *outputWindowThread(void *);
