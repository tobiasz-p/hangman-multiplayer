#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define PORT        8780
#define MAX_CLIENTS 6

#define BUF_SIZE    1024

#include "client.h"

void run(void);

int str_equals(char *, char *);

int init_connection(void);

int read_client(SOCKET sock, char *buffer);

void write_client(SOCKET sock, const char *buffer);

void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);

void broadcast_message(Client clients[MAX_CLIENTS], int actual, const char *buffer);

void remove_client(Client *clients, int to_remove, int *actual);

void clear_clients(Client *clients, int actual);

char ** readFile(int fileDescriptor, int * wordsTotal);

int openFile(const char * path);

char * get_dashed_word(char * word);

#endif
