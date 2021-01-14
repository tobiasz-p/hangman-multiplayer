#include <stdbool.h> 

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
