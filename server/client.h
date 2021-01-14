#include <stdbool.h> 

#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    SOCKET sock;
    char name[BUF_SIZE];
    int available_tries;
    int points;
    int round_points;
    bool is_ready;
} Client;

#endif
