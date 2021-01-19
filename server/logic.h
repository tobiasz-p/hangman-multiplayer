#ifndef HANGMAN_LOGIC_H
#define HANGMAN_LOGIC_H
int checkAnswer(char letterTyped, char * word, char * dashedWord);

Client get_winner(Client *clients, int all_players);

void reset_players(Client clients[MAX_CLIENTS], int actual);

void reset_round_points(Client clients[MAX_CLIENTS], int actual);

void set_all_not_ready(Client clients[MAX_CLIENTS], int actual, int *ready_players);

void set_all_ready(Client clients[MAX_CLIENTS], int actual, int *ready_players);

bool allLost(Client clients[MAX_CLIENTS], int all);

#endif //HANGMAN_LOGIC_H
