
#define PORT 5000
#define CLEAR_WINDOW 1
#define NOT_CLEAR_WINDOW 0

int InitChatClient(struct sockaddr_in server_addr, struct hostent *host);
void InitializeChatWindows(char * buf);
