#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* Exit status codes */
#define ERROR_BAD_ARGS 1
#define ERROR_PLAYER_INVALID 2
#define ERROR_DECK_READ 3
#define ERROR_SAVE_READ 4
#define ERROR_SHORT_DECK 5
#define ERROR_FULL_BOARD 6
#define ERROR_END_HUMAN_INPUT 7

/* Status Reads */
#define NEW_GAME 1
#define END_GAME 0
#define MIDDLE_GAME 2

/* Buffers */
#define CHAR_BUFFER 30

/* Player Constants */
#define HAND_SIZE 6
#define NUM_PLAYERS 2
#define PLAYER_ONE 0
#define PLAYER_TWO 1
#define TURN_ONE 1
#define TURN_TWO 2

/* Macros to read cards in a for loop */
#define GET_NUM(x) (2 * x)
#define GET_SUIT(x) (2 * x + 1)

/* Stores a single Card.
 *
 * @param num - The cards number (1 - 9)
 * @param suit - The cards suit (A - Z)
 */
typedef struct {
    char num;
    char suit;
} Card;

/* Stores Card structs
 *
 * @param cards - An array of cards
 * @param length - The number of cards
 */
typedef struct {
    Card* cards;
    int length;
} CardArray;


/* The location of all necessary game variables
 *
 * @param width - board width
 * @param height - board height
 * @param turn - Players turn, either 1 or 2
 * @param status - NewGame, EndGame, MiddleGame
 * @param boar - Stores all cards
 * @param deckFile - the deck file name
 * @param cardsDrawn - number of cards pulled from the deck
 * @param deck - An array to store the deck in
 * @param hands - An array to store the players hands in.
 */
typedef struct {
    int width;
    int height;
    int turn;
    int status;
    char playerType[NUM_PLAYERS];
    Card** board;
    char* deckFile;
    int cardsDrawn;
    CardArray deck;
    CardArray hands[NUM_PLAYERS];
} Game;

/* File + Argument parsing function */
char* read_line(FILE* f, char** line);
int read_int(char* line);
char check_player(char* line);
int check_dimension(char* line);
void parse_save_file(Game* game, char* fileName);
void parse_deck_file(Game* game);
void parse_line_one(char* line, Game* game); 
void parse_hands(Game* game, char* line, int player);
void parse_board(Game* game, char* line, int lineN);
bool check_spaces(char* line, int spaceReq);
bool check_card(char suit, char num);

/* Game Running functions */
void exit_game(int exitCode);
void game_loop(Game* game);
void player_handler(Game* game);
void calc_scores(Game* game);

/* Game Helper functions*/
void save_game(Game* game, char* fileName);
void make_move(Game* game, int x, int y, int c);
void return_move(Game* game);
void get_path_length(Game* game, int row, int col, int length, 
        int* longest, char charToCheck);
bool adjacent_to(Game*, int x, int y);
bool board_full(Game* game);
bool check_input(Game* game, char* line);
bool check_entry(Game* game, char* line);
bool get_neighbour(Game* game, Card c, int row, int col,
        int** coords, int* size);

/* Output Functions */
void print_board(Game* game);
void print_deck(Game* game);

/* Setup Functions */
bool deal_cards(Game* game);
void malloc_var(Game* game);

int main(int argc, char** argv) {
    Game game;
    CardArray deck;
    CardArray h1;
    CardArray h2;
    game.deck = deck;
    game.hands[PLAYER_ONE] = h1;
    game.hands[PLAYER_TWO] = h2;
    if (argc == 4) {
        // Loading from a save
        game.playerType[PLAYER_ONE] = check_player(argv[2]);
        game.playerType[PLAYER_TWO] = check_player(argv[3]);
        parse_save_file(&game, argv[1]);
    } else if (argc == 6) {
        // Loading from scratch
        game.deckFile = argv[1];
        game.width = check_dimension(argv[2]);
        game.height = check_dimension(argv[3]);
        game.playerType[PLAYER_ONE] = check_player(argv[4]);
        game.playerType[PLAYER_TWO] = check_player(argv[5]);
        game.cardsDrawn = 0;
        parse_deck_file(&game);
        game.status = NEW_GAME;
        game.hands[PLAYER_ONE].length = 0;
        game.hands[PLAYER_TWO].length = 0;
        game.turn = PLAYER_ONE;
        malloc_var(&game);
        if (!deal_cards(&game)) {
            exit_game(ERROR_SHORT_DECK);
        }
        game.turn = TURN_ONE;
    } else {
        exit_game(ERROR_BAD_ARGS);
    }
    game_loop(&game);
    exit_game(0);
    return 0;
}

/* Read a line of text
 *
 * @param f The stream to read from
 * @param line A variable to save to
 * @return The line that is read
 */
char* read_line(FILE* f, char** line) {
    int c;
    int lineL = 0;
    int charCount = CHAR_BUFFER;
    *line = malloc(sizeof(char) * charCount);
    while ((c = fgetc(f)) != '\n') {
        if (c == EOF) {
            free(*line);  
            return NULL;
        }

        (*line)[lineL++] = c;
        if (lineL + 1 >= charCount) {
            charCount *= 2;
            *line = realloc(*line, sizeof(char) * charCount);
        }
    }

    (*line)[lineL] = '\0';
    return *line;
}

/* Convert some characters into an integer.
 * Returns -1 if this fails.
 *
 * @param line - The characters to turn into an integer.
 */
int read_int(char* line) {
    char* pointTo;
    int num;
    num = strtol(line, &pointTo, 10);
    if (strlen(pointTo) > 0) {
        // Any non-integer characters read
        return -1;
    }

    return num;
}

/* Check if the argument passed to player is correct.
 *
 * @param line The user input string.
 */
char check_player(char* line) {
    if (strlen(line) != 1 || (line[0] != 'h' && line[0] != 'a')) {
        exit_game(ERROR_PLAYER_INVALID);
    }

    return line[0];
}

/* Check if the provided width / height is to specification
 *
 * @param line The user input string
 */
int check_dimension(char* line) {
    int len = read_int(line);
    if (len < 3 || len > 100) {
        exit_game(ERROR_PLAYER_INVALID);
    }

    return len;
}

/* Read from the save File, catching any errors
 *
 * @param game - information about the game state
 * @param fileName - The name of the file to read from
 */
void parse_save_file(Game* game, char* fileName) {
    FILE* f;
    f = fopen(fileName, "r");
    if (!f) {
        exit_game(ERROR_SAVE_READ);
    }

    int lineN = 0;
    char* line;
    while (read_line(f, &line)) {
        switch(lineN) {
            case 0:
                parse_line_one(line, game);
                break;
            case 1:
                game->deckFile = line;
                parse_deck_file(game);
                break;
            case 2:
            case 3:
                parse_hands(game, line, lineN - 1);
                break;
            default:
                parse_board(game, line, lineN - 4);
        }
        free(line);
        lineN++;
    }
    // Check that all lines are read and the board height is correct.
    if (lineN != game->height + 4) {
        exit_game(ERROR_SAVE_READ);
    } else if (board_full(game)) {
        exit_game(ERROR_FULL_BOARD);
    }
    fclose(f);
}

/* Check that the first line of save file is valid and populate the game
 *
 * @param line - the line to analyse
 * @param game - information about the game state
 */
void parse_line_one(char* line, Game* game) {
    if (!check_spaces(line, 3)) {
        exit_game(ERROR_SAVE_READ);
    }

    game->width = check_dimension(strtok(line, " "));
    game->height = check_dimension(strtok(NULL, " "));
    game->cardsDrawn = read_int(strtok(NULL, " "));
    game->turn = read_int(strtok(NULL, " "));
    if ((game->turn != TURN_ONE && game->turn != TURN_TWO) 
            || game->cardsDrawn < 11) {
        // Must be at least 11 cards allocated at the start
        exit_game(ERROR_SAVE_READ);
    }

    game->status = NEW_GAME;
    malloc_var(game);
}

/* Read the deck file, checking for errors.
 *
 * @param game - information about the game state
 */
void parse_deck_file(Game* game) {
    FILE* f;
    f = fopen(game->deckFile, "r");
    if (!f) {
        exit_game(ERROR_DECK_READ);
    }

    char* line;
    int lineN = 0;
    if (!read_line(f, &line) || (game->deck.length = read_int(line)) <= 0 
            || game->deck.length < game->cardsDrawn) {
        exit_game(ERROR_DECK_READ);
    }

    free(line);
    game->deck.cards = malloc(sizeof(Card) * game->deck.length);
    while (read_line(f, &line)) {
        if (strlen(line) != 2 || lineN >= game->deck.length 
                || !check_card(line[1], line[0])) {
            exit_game(ERROR_DECK_READ);
        }

        Card temp;
        temp = (Card){.num = line[0], .suit = line[1]};
        game->deck.cards[lineN] = temp;
        lineN++;
        free(line);
    }
    fclose(f);
    if (lineN != game->deck.length) {
        exit_game(ERROR_DECK_READ);
    }
}

/* Read a players hand, checking for errors.
 *
 * @param game - information about the game state
 * @param line - A string to analyse
 * @param player - The player to assign the hand to
 */
void parse_hands(Game* game, char* line, int player) {
    int length = strlen(line);
    if (length != HAND_SIZE * 2 && length != (HAND_SIZE - 1) * 2) {
        exit_game(ERROR_SAVE_READ);
    }

    game->hands[--player].length = length / 2;
    for (int i = 0; i < game->hands[player].length; i++) {
        Card temp;
        temp = (Card){.num = line[GET_NUM(i)], .suit = line[GET_SUIT(i)]};
        if (!check_card(temp.suit, temp.num)) {
            exit_game(ERROR_SAVE_READ);
        }

        game->hands[player].cards[i] = temp;
    }
}

/* Read a row of the board, checking for errors
 *
 * @param game - information about the game state
 * @param line - A row of the board
 * @param lineN - The row number that the line is to go on
 */
void parse_board(Game* game, char* line, int lineN) {
    int length = strlen(line);
    if (length != game->width * 2) {
        // Two chars per card
        exit_game(ERROR_SAVE_READ);
    }

    for (int i = 0; i < length / 2; i++) {
        Card temp;
        temp = (Card){.num = line[GET_NUM(i)], .suit = line[GET_SUIT(i)]};
        if (!check_card(temp.suit, temp.num)) {
            exit_game(ERROR_SAVE_READ);
        }
        // Check that if a card has been played.
        if (game->status == NEW_GAME && temp.suit != '*') {
            game->status = MIDDLE_GAME;
        }
        game->board[lineN][i] = temp;
    }
}

/* Exits the game with specifid error Code
 *
 * @param exitCode - what to exit with
 */
void exit_game(int exitCode) {
    switch (exitCode) {
        case ERROR_BAD_ARGS:
            fprintf(stderr, "Usage: bark savefile p1type p2type\n");
            fprintf(stderr, "bark deck width height p1type p2type\n");
            break;
        case ERROR_PLAYER_INVALID:
            fprintf(stderr, "Incorrect arg types\n");
            break;
        case ERROR_DECK_READ:
            fprintf(stderr, "Unable to parse deckfile\n");
            break;
        case ERROR_SAVE_READ:
            fprintf(stderr, "Unable to parse savefile\n");
            break;
        case ERROR_SHORT_DECK:
            fprintf(stderr, "Short deck\n");
            break;
        case ERROR_FULL_BOARD:
            fprintf(stderr, "Board full\n");
            break;
        case ERROR_END_HUMAN_INPUT:
            fprintf(stderr, "End of input\n");
            break;
    }
    exit(exitCode);
}

/* Run the game. Dealing cards, collecting and displaying
 * moves and score.
 *
 * @param game - information about the game state
 */
void game_loop(Game* game) {
    while (game->status && !board_full(game) && deal_cards(game)) {
        print_board(game);
        player_handler(game);
        game->turn = (game->turn == TURN_ONE) ? TURN_TWO : TURN_ONE;
    }
    print_board(game);
    calc_scores(game);
}

/* Read player input and make a move
 *
 * @parm game - information about the game state
 */
void player_handler(Game* game) {
    print_deck(game);
    if (game->playerType[game->turn - 1] == 'a') {
        return_move(game);
        return;
    }
    char* line;
    while (1) {
        printf("Move? ");
        if (!read_line(stdin, &line)) {
            exit_game(ERROR_END_HUMAN_INPUT);
        } else if (check_input(game, line)) {
            break;
        }
        free(line);
    }
    free(line);
}

/* Check if the players move is valid;
 *
 * @param game - information about the game state
 * @parmam line - the players input.
 */
bool check_input(Game* game, char* line) {
    int length = strlen(line);
    char save[5];
    if (length < 5) {
        // Both moves and SAVEs are longer than 5
        return false;
    }

    strncpy(save, line, 4);
    save[4] = '\0';
    // strcmp returns 0 if there is a match
    if (strcmp(save, "SAVE")) {
        return check_entry(game, line);
    } else {
        save_game(game, line + 4);
        return false;
    }
}

/* Check that a line of input has the required amount of spaces
 *
 * @param line - the string to analyse
 * @param spaceReq - the number of spaces to check for
 */
bool check_spaces(char* line, int spaceReq) {
    int spaces = 0;
    for (int i = 0; i < strlen(line) - 1; i++) {
        if (line[i] == ' ') {
            // No consecutive spaces
            if (line[i + 1] == ' ') {
                return false;
            }
            spaces++;
        }
    }
    return (spaces == spaceReq);
}

/* Make sure the player has entered a valid move
 *
 * @param line - the players move
 * @param game - information about the game state
 */
bool check_entry(Game* game, char* line) {
    if (!check_spaces(line, 2)) {
        return false;
    }

    int card = read_int(strtok(line, " "));
    int col = read_int(strtok(NULL, " "));
    int row = read_int(strtok(NULL, " "));
    if (card < 1 || card > HAND_SIZE || col < 1 || col > game->width || row < 1
            || row > game->height
            || !adjacent_to(game, row - 1, col - 1)) {
        // Make sure user input is valid
        return false;
    }

    make_move(game, row - 1, col - 1, card - 1);
    return true;
}

/* Save the game to a file
 *`
 * @param game - information about the game state
 * @param fileName - the file to save to
 */
void save_game(Game* game, char* fileName) {
    for (int i = 0; i < strlen(fileName); i++) {
        // Check for at least one character
        if ((fileName[i] >= 65 && fileName[i] <= 90) || 
                (fileName[i] >= 97 && fileName[i] <= 122)) {
            break;    
        } else if (i + 1 == strlen(fileName)) {
            printf("Unable to save\n");
            return;            
        }
    }

    FILE* f = fopen(fileName, "w");
    if (!f) {
        printf("Unable to save\n");
        return;
    }

    fprintf(f, "%d %d %d %d\n", game->width, game->height,
            game->cardsDrawn, game->turn);
    fprintf(f, "%s\n", game->deckFile);
    int length;
    for (int i = 0; i < NUM_PLAYERS; i++) {
        length = (game->turn == i + 1) ? HAND_SIZE : HAND_SIZE - 1;
        for (int j = 0; j < length; j++) {
            fprintf(f, "%c%c", game->hands[i].cards[j].num,
                    game->hands[i].cards[j].suit);
        }
        fprintf(f, "\n");
    }

    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            fprintf(f, "%c%c", game->board[i][j].num, game->board[i][j].suit);
        }
        fprintf(f, "\n");
    }
}

/* Make a move on the game board
 *
 * @param game - information about the game state
 * @param row - row to place the card
 * @param col - column to place the card
 * @param c - position of card in hand 1 <= c <= HAND_SIZE
 */
void make_move(Game* game, int row, int col, int c) {
    int turn = game->turn - 1;
    Card temp = game->hands[turn].cards[c];
    game->hands[turn].length--;
    for (int k = c; k < HAND_SIZE - 1; k++) {
        // Shuffle cards down.
        game->hands[turn].cards[k] = game->hands[turn]
                .cards[k + 1];
    }
    game->board[row][col].num = temp.num;
    game->board[row][col].suit = temp.suit;
    game->status = (game->status == NEW_GAME) ? MIDDLE_GAME : game->status;
}

/* Calculate the players scores.
 *
 * @param game - information about the game state
 */
void calc_scores(Game* game) {
    int pLength[NUM_PLAYERS] = {0, 0}; // Default length of 0.
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            if (game->board[i][j].suit == '*') {
                continue;
            }
            // Find the longest path from the current position
            get_path_length(game, i, j, 1, 
                    &pLength[(game->board[i][j].suit % 2 != 0) ? 
                    PLAYER_ONE : PLAYER_TWO], game->board[i][j].suit);
        }
    }
    printf("Player 1=%d Player 2=%d\n", 
            pLength[PLAYER_ONE], pLength[PLAYER_TWO]);
}

/* A recursive call to find the length of a path
 *
 * @param row - board row to check
 * @param col - board column to check
 * @param length - Size of current path
 * @param longest - Reference to var storing current longest path
 * @param charToCheck - Which char to end with
 */
void get_path_length(Game* game, int row, int col, int length, 
        int* longest, char charToCheck) {
    Card c = game->board[row][col];
    int* coords;
    int coordSize;
    if (charToCheck == c.suit) {
        // Update the longest path when a matching suit is arrived at.
        *longest = ((*longest) < length) ? length : (*longest);
    }

    if (get_neighbour(game, c, row, col, &coords, &coordSize)) {
        // Check if neighbouring cards are accessible and recur.
        length++;
        for (int i = 0; i < coordSize; i++) {
            get_path_length(game, coords[2 * i], coords[2 * i + 1],
                    length, longest, charToCheck);
        }
    }
    free(coords);
}

/* Save the coordinates and how many to check
 *
 * @param c - The Card to compare
 * @param row - board[row]
 * @param col - board[row][col]
 * @param coords - Where to save the coordinates
 * @param size - The size of the coords array.
 */
bool get_neighbour(Game* game, Card c, int row, int col, int** coords,
        int* size) {
    *coords = malloc(sizeof(int) * 8); // 4 possible directions -> 8 ints
    int temp, temp2;
    int x = 1;
    int y = 0; // check (1, 0) first.
    *size = 0; // start at 0.
    int j = 4; // # sides to check
    while ((*size) < j) {
        // Load in coordinates, treating them as if the board is a torus.
        temp = (row + x == -1) ? game->height - 1 : (row + x) % game->height;
        temp2 = (col + y == -1) ? game->width - 1 : (col + y) % game->width;
        if (game->board[temp][temp2].num > c.num &&
                game->board[temp][temp2].suit != '*') {
            // Save the coordinates if they are valid.
            (*coords)[2 * (*size)] = temp;
            (*coords)[2 * (*size) + 1] = temp2;
            (*size)++;
        } else {
            j--;
        }
        // Set the next values to check.
        temp = -y;
        y = (x == 0) ? 0 : x;
        x = (x == 0) ? temp : 0;
    }
    return (j == 0) ? false : true; // Did we read any coords?
}

/* Check if a card is valid
 *
 * @param suit - The cards letter value
 * @param num - The cards numerical value
 */
bool check_card(char suit, char num) {
    if (suit == '*' && num == '*') {
        return true;
    } else if (suit > 90 || suit < 65 || num > 57 || num < 49) {
        return false;
    }
    return true;
}

/* Make an AI move.
 *
 * @param game - information about the game state.
 */
void return_move(Game* game) {
    int player = game->turn - 1;
    Card temp = game->hands[player].cards[0]; // Always select first card
    int column;
    int row;
    if (game->status == NEW_GAME) {
        row = (game->height - 1) / 2;
        column = (game->width - 1) / 2;
    } else {
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                row = (player != PLAYER_ONE) ? game->height - i - 1 : i;
                column = (player != PLAYER_ONE) ? game->width - j - 1 : j;
                if (adjacent_to(game, row, column)) {
                    // exit the loops
                    i = game->height;
                    j = game->width;
                }
            }
        }
    }
    make_move(game, row, column, 0);
    printf("Player %d plays %c%c in column %d row %d\n", player + 1, temp.num,
            temp.suit, column + 1, row + 1);
}

/* Check if the provided position is adjacent to a Card
 *
 * @param game - information about the game state
 * @param x - row
 * @param y - column
 */
bool adjacent_to(Game* game, int x, int y) {
    if (game->status == NEW_GAME) {
        // A card is always valid for an empty board
        return true;
    } else if (game->board[x][y].suit != '*') {
        // Make sure the space is empty
        return false;
    }

    int row;
    int column;
    for (int j = -1; j < 2; j++) {
        for (int i = -1; i < 2; i++) {
            if (abs(i) == abs(j)) {
                // Dont check diagonals
                continue;
            }
            row = (x + j == -1) ? game->height - 1 : (x + j) % game->height;
            column = (y + i == -1) ? game->width - 1 : (y + i) % game->width;
            if (game->board[row][column].suit != '*') {
                return true;
            }
        }
    }
    return false;
}

/* Check if the board is full of cards and end the game if it is.
 *
 * @param game - information about the game state
 */
bool board_full(Game* game) {
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            if (game->board[i][j].suit == '*') {
                return false;
            }
        }
    }
    game->status = END_GAME;
    return true;
}

/* Prints the board to the console.
 *
 * @param game - information about the game state
 */
void print_board(Game* game) {
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            if (game->board[i][j].suit == '*') {
                printf("..");
            } else {
                printf("%c%c", game->board[i][j].num, game->board[i][j].suit);
            }
        }
        printf("\n");
    }
}

/* Output the deck to stdout
 *
 * @param game - information about the game state
 */
void print_deck(Game* game) {
    int turn = game->turn - 1;
    printf("Hand");

    if (game->playerType[turn] == 'h') {
        printf("(%d)", game->turn);
    }

    printf(":");

    for (int i = 0; i < 6; i++) {
        printf(" %c%c", game->hands[turn].cards[i].num,
                game->hands[turn].cards[i].suit);
    }

    printf("\n");
}

/* Deal cards to each player.
 *
 * @param game - information about the game state
 */
bool deal_cards(Game* game) {

    for (int i = 0; i < NUM_PLAYERS; i++) {
        for (int j = game->hands[i].length; j < HAND_SIZE; j++) {
            // Only give the current turn player 6 cards.
            if (j == HAND_SIZE - 1 && i + 1 != game->turn) {
                break;
            }
            game->hands[i].cards[j] = game->deck.cards[game->cardsDrawn++];
            game->hands[i].length++;

            if (game->deck.length < game->cardsDrawn) {
                return false;
            }
        }
    }

    return true;
}

/* Allocate memory to board and hands;
 *
 * @param game - information about the game state
 */
void malloc_var(Game* game) {

    game->board = malloc(sizeof(Card*) * game->height);

    for (int i = 0; i < game->height; i++) {
        game->board[i] = malloc(sizeof(Card) * game->width);
        for (int j = 0; j < game->width; j++) {
            Card temp;
            temp = (Card){.num = '*', .suit = '*'};
            game->board[i][j] = temp;
        }
    }

    game->hands[PLAYER_ONE].cards = malloc(sizeof(Card) * HAND_SIZE);
    game->hands[PLAYER_TWO].cards = malloc(sizeof(Card) * HAND_SIZE);
}

