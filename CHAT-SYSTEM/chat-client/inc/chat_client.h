
#define PORT 5000
#define CLEAR_WINDOW 1
#define NOT_CLEAR_WINDOW 0

void InitializeChatWindows();
int InitializeChatSocket(struct sockaddr_in server_addr, struct hostent *host);

void *inputWindowThread(void *);
void *outputWindowThread(void *);
