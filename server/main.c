#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>  
#include <unistd.h>   
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#include "server.h"

#define TRIES 6

int ready_players = 0;
int fds[2];
int init_game = 1;

static volatile int keep_running = 1;

int checkAnswer(char letterTyped, char * word, char * dashedWord) {
    size_t wordLength = strlen(word);
    int replacedLetters = 0;

    for (int i = 0; i < wordLength; i++) {
        if (word[i] == letterTyped) {
            if (dashedWord[i] == '_') { // Does it need to be replaced?
                dashedWord[i] = letterTyped;
                replacedLetters++;
            }
        }
    }
    return replacedLetters;
}

Client get_winner(Client *clients, int all_players) {
    int winners = 0;
    Client winner;
    winner.points=-1;
    for (int i = 0; i < all_players; i++) {
        if (clients[i].points >= 4*all_players && clients[i].points > winner.points){
            winner = clients[i];
            winners = 1;
        }
        else if (clients[i].points == winner.points){ // ex aequo
            winners ++;
        }
    }
    if (winners > 1){
        winner.points=-1;
        // TODO: return NULL (?)
        return winner; // no winner
    }
    return winner;
}

void reset_players(Client clients[MAX_CLIENTS], int actual){
    for (int i = 0; i < actual; i++) {
        clients[i].points=0;
        clients[i].is_ready = false;
    }
}

void reset_round_points(Client clients[MAX_CLIENTS], int actual){
    for (int i = 0; i < actual; i++) {
        clients[i].round_points=0;
    }
}

void set_all_not_ready(Client clients[MAX_CLIENTS], int actual, int *ready_players){
    for (int i=0; i<actual; i++){
        clients[i].is_ready=false;
    }
    *ready_players = 0;
}

void set_all_ready(Client clients[MAX_CLIENTS], int actual, int *ready_players){
    for (int i=0; i<actual; i++){
        clients[i].is_ready=true;
    }
    *ready_players = actual;
}

bool allLost(Client clients[MAX_CLIENTS], int all){
    int losers = 0;
    for(int i = 0; i < all; i++){
        if(clients[i].available_tries < 1){
            losers++;
        }
    }
    if(losers == all){
        return true;
    }
    return false;
}


void* wait(){
    init_game=2;
    puts("Game will start in 10s...");
    sleep(10);
    puts("Game is starting");
    write(fds[1], "go", 3);
    init_game=0;
    return NULL;
}

void sig_handler(){
  printf("Timeout\n");
}

void int_handler() {
    keep_running = 0;
    puts("Server closed");
}

void winner_message(Client clients[MAX_CLIENTS], Client winner, int actual){
    alarm(0);
    char buffer[BUF_SIZE];
    strncpy(buffer, "# winner# ", BUF_SIZE - 1);
    strncat(buffer, winner.name, BUF_SIZE - strlen(buffer) - 1);
    broadcast_message(clients, actual, buffer);
    printf("The winner is %s", winner.name);
}


void run() {
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;
    int max = sock;
    Client clients[MAX_CLIENTS];    // An array for all clients
    int words_total = 0, words_fd;    // Used to load word list
    char ** wordsList;
    char selected_word[100];
    char hidden_word[100];
    char wrong_letters[26] ="\0";
    int game_start = 1;
    int flags;
    pthread_t thread;
    fd_set rdfs;

    while(keep_running) { // main loop
        while (keep_running) { // match loop
            if(init_game == 1 && actual >= 1 && ready_players >= 2){

                Client winner = get_winner(clients, ready_players); // check winner
                if (winner.points != -1){
                    winner_message(clients, winner, actual);
                    break;
                }
                broadcast_message(clients, actual, "# message# Round will start in 10s");
                pthread_create (&thread, NULL, wait, NULL);
                pthread_detach(thread);

                if (game_start){ // first round of the game
                    set_all_ready(clients, actual, &ready_players);
                    printf("ready: %d\n", ready_players);
                    printf("actual: %d\n", actual);
                    for (int i =0; i < actual; i++){ // send all users in game
                        strncpy(buffer, "# user", BUF_SIZE - 1);
                        char str[7];
                        sprintf(str, "# %d# ", i);
                        strncat(buffer, str, BUF_SIZE - 1);
                        strncat(buffer, clients[i].name, BUF_SIZE - strlen(buffer) - 1);
                        broadcast_message(clients, actual, buffer);                    
                    }
                    
                    game_start = 0;
                }
            }
            if (init_game ==0){
                // Initialize the round
                words_fd = openFile("../words.txt");
                wordsList = readFile(words_fd, &words_total);
                close(words_fd);
                srand(time(NULL));
                int r = rand() % words_total;

                broadcast_message(clients, actual, "# reset");
                broadcast_message(clients, actual, "# message# Let's the round begin!");
                strcpy(selected_word, wordsList[r]);
                strcpy(hidden_word, get_dashed_word(selected_word));
                printf("Selected word %s", selected_word);
                //puts("\n");

                reset_round_points(clients, actual);
                for (int i=0; i<actual; i++){ // send actual points
                    points_to_buffer(clients[i], i, buffer);
                    broadcast_message(clients, actual, buffer);
                }
                
                for (int i = 0; i < actual; i++) {
                    if (clients[i].is_ready){
                        clients[i].available_tries = TRIES;
                    }
                }
                strcpy(wrong_letters, "\0");
                init_game = -1;
            }
            if (init_game == -1){ // round in progress
                if (allLost(clients, ready_players)){
                   // broadcast_message(clients, actual, "# all_lost");
                    init_game = 1;
                    continue;
                }
                else if (ready_players < 2){ // only one player left, other disconnected
                    init_game = 1;
                    for (int i=0; i < actual; i++){
                        if(clients[i].is_ready){
                            winner_message(clients, clients[i], actual); // it is the only player left
                            break;
                        }
                    }
                    break;
                }
                strncpy(buffer, "# hidden# ", BUF_SIZE-1);
                strncat(buffer, hidden_word, BUF_SIZE - strlen(buffer) -1);
                broadcast_message(clients, actual, buffer);
    
                strncpy(buffer, "# wrong_letters# ", BUF_SIZE-1);
                strncat(buffer, wrong_letters, BUF_SIZE - strlen(buffer) -1);
                broadcast_message(clients, actual, buffer);
            }

            if (pipe(fds) == -1)
                perror("pipe error");

            // select
            FD_ZERO(&rdfs);
            FD_SET(STDIN_FILENO, &rdfs);
            FD_SET(fds[0], &rdfs);
            max = fds[0] > max ? fds[0] : max;    // set the new max fd

            /* Make read and write ends of pipe nonblocking */
            flags = fcntl(fds[0], F_GETFL);
            if (flags == -1)
                perror("fcntl-F_GETFL");
            flags |= O_NONBLOCK;                /* Make read end nonblocking */
            if (fcntl(fds[0], F_SETFL, flags) == -1)
                perror("fcntl-F_SETFL");

            flags = fcntl(fds[1], F_GETFL);
            if (flags == -1)
                perror("fcntl-F_GETFL");
            flags |= O_NONBLOCK;                /* Make write end nonblocking */
            if (fcntl(fds[1], F_SETFL, flags) == -1)
                perror("fcntl-F_SETFL");

            FD_SET(sock, &rdfs);
            for (int i = 0; i < actual; i++)
                FD_SET(clients[i].sock, &rdfs); // Adding each client's socket

            if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
                if (errno == EINTR){ // handle timeout
                    broadcast_message(clients, actual, "# timeout");
                    init_game = 1;
                    alarm(0);
                    continue;
                }
                else{
                    perror("select error");
                    exit(errno);
                }
            }

            if (FD_ISSET(STDIN_FILENO, &rdfs)){
                break;
            }

            char ch;
            if (FD_ISSET(fds[0], &rdfs)) {   // Handler was called
                puts("A signal was caught\n");
                for (;;) { // Consume bytes from pipe
                    if (read(fds[0], &ch, 1) == -1) {
                        if (errno == EAGAIN){ // No more bytes
                            break;
                        }
                        else
                            perror("read"); // Some other error
                    }
                }
            }

            else if (FD_ISSET(sock, &rdfs)) { // New connection
                SOCKADDR_IN csin = {0};
                size_t sinsize = sizeof(csin);
                int csock = accept(sock, (SOCKADDR *) &csin, (socklen_t *restrict) &sinsize);
                if (csock == SOCKET_ERROR) {
                    perror("accept()");
                    continue;
                }
                if (actual < MAX_CLIENTS){
                    if (read_client(csock, buffer) == -1)   // After connecting, the client sends its name
                        continue; // Disconnects

                    max = csock > max ? csock : max;    // returns the new max fd

                    FD_SET(csock, &rdfs);
                    Client c;
                    c.sock = csock;
                    strncpy(c.name, buffer, BUF_SIZE - 1);

                    strncpy(buffer, "# connected", BUF_SIZE - 1);
                    char str[6];
                    sprintf(str, "# %d# ", actual);
                    strncat(buffer, str, BUF_SIZE - strlen(buffer) - 1);
                    strncat(buffer, c.name, strlen(buffer) - BUF_SIZE - 1);
                    broadcast_message(clients, actual, buffer);
                    printf("%s connected\n", c.name);
                    if (game_start == -1){
                        c.available_tries = 6;
                    }
                    else{
                        c.available_tries = 0;
                    }
                    c.points = 0;
                    c.is_ready = false;
                    clients[actual] = c;
                    actual++;
                }
                else{
                    strncpy(buffer, "# room_full", BUF_SIZE - 1);
                    write_client(csock, buffer);
                    close_socket(csock);
                }

            }
            else{
                for (int i = 0; i < actual; i++) {
                    if (FD_ISSET(clients[i].sock, &rdfs)) { // A client is talking
                        Client client = clients[i];
                        int c = read_client(clients[i].sock, buffer);
                        if (c == 0) {                       // A client disconnected
                            close_socket(clients[i].sock);
                            if (client.is_ready)
                               { ready_players--; }
                            remove_client(clients, i, &actual);
                            strncpy(buffer, "# dc", BUF_SIZE - 1);
                            char str[4];
                            sprintf(str, "# %d", i);
                            strncat(buffer, str, BUF_SIZE - 1);
                            printf("Client %d disconnected", i);
                            broadcast_message(clients, actual, buffer);
                        }
                        else if (str_equals(buffer, "ready")){
                            if(!clients[i].is_ready){
                                clients[i].is_ready = true;
                                ready_players++;
                                printf("%s is ready\n", clients[i].name);
                            }
                        }
                        else if (client.available_tries > 0 && init_game == -1 && client.is_ready){
                            int replacedLetters = checkAnswer(tolower(buffer[0]), selected_word, hidden_word);
                            alarm(20);
                            // Check results
                            if (replacedLetters == 0) {
                                // No letter has been replaced == wrong answer
                                if (strpbrk(wrong_letters, buffer)==0 && strpbrk(hidden_word, buffer)==0) {
                                    append(wrong_letters, buffer[0]);
                                    strncpy(buffer, "# available_tries", BUF_SIZE - 1);
                                    char str[4];
                                    sprintf(str, "# %d", i);
                                    strncat(buffer, str, BUF_SIZE - strlen(buffer) - 1);
                                    sprintf(str, "# %d", --clients[i].available_tries);
                                    strncat(buffer, str, BUF_SIZE - strlen(buffer) - 1);
                                }

                            }
                            else {
                                // Good guess
                                clients[i].points++;
                                clients[i].round_points++;
                                points_to_buffer(clients[i],i, buffer);
                                broadcast_message(clients, actual, buffer);
                            }

                            // Check the game state
                            if (strcmp(hidden_word, selected_word) == 0) {
                                alarm(0);
                                strncat(buffer, "# hidden# ", BUF_SIZE - strlen(buffer) - 1);
                                strncat(buffer, selected_word, BUF_SIZE - strlen(buffer) - 1);
                                init_game = 1;
                            }

                            if (clients[i].available_tries == 0) {
                                clients[i].points-=3;
                                points_to_buffer(clients[i],i, buffer);
                                broadcast_message(clients, actual, buffer);
                            }
                            broadcast_message(clients, actual, buffer);
                        }
                        else if (client.available_tries > 0 && init_game != -1){
                            strncpy(buffer, "# not_started", BUF_SIZE - 1);
                            write_client(client.sock, buffer);
                        }
                        else{ // Client doesn't have more available tries
                            strncpy(buffer, "# wait", BUF_SIZE - 1);
                            write_client(client.sock, buffer);
                        }
                        break;
                    }
                }
            }
        }
        init_game = 1;
        set_all_not_ready(clients, actual, &ready_players);
        reset_players(clients, actual);
        printf("ready: %d", ready_players);
        game_start = 1;
    }
    clear_clients(clients, actual);
    close_socket(sock);
    puts("Server closed");
}


void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}


void points_to_buffer(Client client,int i, char *buffer){
    char str[4];
    strncpy(buffer, "# points", BUF_SIZE - 1);
    sprintf(str, "# %d", i);
    strncat(buffer, str, BUF_SIZE - strlen(buffer) - 1);
    sprintf(str, "# %d", client.points);
    strncat(buffer, str, BUF_SIZE - strlen(buffer)- 1);
}

char * get_dashed_word(char * word) {
    size_t wordLength = strlen(word);

    char * hidden_word = malloc(wordLength * sizeof(char));
    for (int i = 0; i < strlen(word); i++) {
        hidden_word[i] = '_';
    }
    return hidden_word;
}

int openFile(const char * path) {
    int fileDescriptor = open(path, O_RDONLY);

    if (fileDescriptor == -1) {
        printf("Error while opening file\n");
        exit(-1);
    }

    return fileDescriptor;
}

char ** readFile(int fileDescriptor, int * words_total) {
    // Internal variables
    char temp = 0;
    int cursor = 0;
    ssize_t bytesRead;

    // Setting up pointers
    char ** wordsList = malloc(20 * sizeof(char*));
    *words_total = 0;

    // Reading inside file
    do {
        bytesRead = read(fileDescriptor, &temp, sizeof(char));

        if (bytesRead == -1) {
            // Error while reading file
            printf("Error while reading file\n");
            exit(-1);
        } else if (bytesRead >= sizeof(char)) {
            if (cursor == 0) {
                // Allocate some memory to catch the word
                wordsList[*words_total] = (char *) malloc(sizeof(char) * 30);
            }

            if (temp == '\n') {
                // Finish the current word and update the word count
                wordsList[*words_total][cursor] = '\0';

                // Reallocate unused memory
                wordsList[*words_total] = (char*)realloc(wordsList[*words_total], (cursor+1)* sizeof(char));

                // Update the word count
                (*words_total)++;

                // Reset the cursor
                cursor = 0;
            } else {
                // Write the letter and update the cursor
                wordsList[*words_total][cursor++] = temp;
            }
        }
    } while (bytesRead >= sizeof(char));

    // Update the word count
    if (*words_total > 0)
        (*words_total)++;

    return wordsList;
}

void clear_clients(Client *clients, int actual) {
    int i = 0;
    for (i = 0; i < actual; i++) {
        close_socket(clients[i].sock);
        printf("Disconnected %s\n", clients[i].name);
    }
} 

int getSO_ERROR(int fd) {
   int err = 1;
   socklen_t len = sizeof err;
   if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
      perror("getSO_ERROR");
   if (err)
      errno = err;              // set errno to the socket SO_ERROR
   return err;
}

void close_socket(int fd) {      // *not* the Windows closesocket()
    if (fd >= 0) {
        getSO_ERROR(fd); // first clear any errors, which can cause close to fail
        if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
            if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
                perror("shutdown");
        if (close(fd) < 0) // finally call close()
            perror("close");
    }
}

void remove_client(Client *clients, int to_remove, int *actual) {
    memmove(clients + to_remove, clients + to_remove + 1,
            (*actual - to_remove - 1) * sizeof(Client));  // We remove the client from the array
    (*actual)--;    // Reducing the number of clients
    
}

void
send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server) {
    int i = 0;
    char message[BUF_SIZE];
    message[0] = 0;
    for (i = 0; i < actual; i++) {
        if (sender.sock != clients[i].sock) {
            if (from_server == 0) {
                strncpy(message, sender.name, BUF_SIZE - 1);
                strncat(message, " : ", sizeof message - strlen(message) - 1);
            }
            strncat(message, buffer, sizeof message - strlen(message) - 1);
            write_client(clients[i].sock, message);
        }
    }
}

void
broadcast_message(Client clients[MAX_CLIENTS], int actual, const char *buffer) {
    for (int i = 0; i < actual; i++) {       
        write_client(clients[i].sock, buffer); 
    }
}


int init_connection(void) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);  // Initializing the socket : AF_INET = IPv4, SOCK_STREAM : TCP
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET) {   // if socket() returned -1
        perror("socket() error");
        exit(errno);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    sin.sin_addr.s_addr = htons(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR) {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

int str_equals(char *str1, char *str2) {
    if (!strcmp(str1, str2))
        return 1;
    return 0;
}


int read_client(SOCKET sock, char *buffer) {
    int n = 0;

    if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        /* if recv error we disconnect the client */
        n = 0;
    }

    buffer[n] = 0;

    return n;
}

void write_client(SOCKET sock, const char *buffer) {
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send error");
        exit(errno);
    }
}

int main() {
    signal(SIGALRM, sig_handler);
    signal(SIGINT, int_handler);
    run();
    return EXIT_SUCCESS;
}
