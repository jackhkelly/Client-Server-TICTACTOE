// Final Project - Jack Kelly - Tic Tac Toe
#include "csapp.h"
#include <stdbool.h>
#include <pthread.h>

#define BOARD_SIZE 3

typedef struct {
    int connfd1;  // Player 1 connection
    int connfd2;  // Player 2 connection
    int best_of_n;
} game_data;

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = ' ';
}

void print_board(char board[BOARD_SIZE][BOARD_SIZE], char *output) {
    sprintf(output, "\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            sprintf(output + strlen(output), "%c ", board[i][j] == ' ' ? '.' : board[i][j]);
        }
        sprintf(output + strlen(output), "\n");
    }
}

int check_winner(char board[BOARD_SIZE][BOARD_SIZE]) {
    // Check rows and columns
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0];
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i];
    }

    // Check diagonals
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0];
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2];

    // Check for a draw
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] == ' ')
                return 0;  // Game is not over

    return 'D';  // Draw
}

void send_message(int fd, const char *message) {
    char buffer[MAXLINE];
    snprintf(buffer, sizeof(buffer), "%s\n", message);  // Add newline
    Rio_writen(fd, (void *)buffer, strlen(buffer));     // Write message
    printf("DEBUG: Message sent: %s\n", buffer);        // Debugging log
}

void *handle_game(void *vargp) {
    game_data *data = (game_data *)vargp;
    rio_t rio1, rio2;
    char buffer[MAXLINE];
    char board[BOARD_SIZE][BOARD_SIZE];
    int scores[2] = {0, 0};
    int current_game = 1;

    Rio_readinitb(&rio1, data->connfd1);
    Rio_readinitb(&rio2, data->connfd2);

    // Notify both players
    send_message(data->connfd1, "Welcome, Player 1! Waiting for Player 2...");
    send_message(data->connfd2, "Welcome, Player 2! Waiting for Player 1 to start the game...");
    printf("Sent welcome messages to both players.\n");

    // Prompt Player 1 for the number of games
    send_message(data->connfd1, "Enter the number of games to play (best of N): ");
    printf("Prompt sent to Player 1: Enter the number of games to play.\n");

    Rio_readlineb(&rio1, buffer, MAXLINE);
    printf("Read input from Player 1: %s\n", buffer);

    // Validate the number of games
    if (sscanf(buffer, "%d", &data->best_of_n) != 1 || data->best_of_n <= 0) {
        send_message(data->connfd1, "Invalid input. Please restart the game.");
        printf("Invalid input from Player 1: %s\n", buffer);
        Close(data->connfd1);
        Close(data->connfd2);
        free(data);
        return NULL;
    }

    // Notify Player 2 about the number of games
    sprintf(buffer, "Best of %d games chosen by Player 1. Let’s start!", data->best_of_n);
    send_message(data->connfd2, buffer);
    printf("Notified Player 2: Best of %d games.\n", data->best_of_n);

    while (current_game <= data->best_of_n) {
        printf("Starting game %d of %d.\n", current_game, data->best_of_n);
        init_board(board);
        int turn = 1;
        int winner = 0;

        while (!winner) {
            rio_t *current_rio = (turn == 0) ? &rio1 : &rio2;
            int current_fd = (turn == 0) ? data->connfd1 : data->connfd2;
            int other_fd = (turn == 0) ? data->connfd2 : data->connfd1;

            sprintf(buffer, "Your turn, Player %d! Enter row and column (e.g., 1 2) ", turn + 1);
            send_message(current_fd, buffer);
            send_message(other_fd, "Waiting for the other player to make a move...");
            printf("Prompted Player %d for their move.\n", turn + 1);

            Rio_readlineb(current_rio, buffer, MAXLINE);
            printf("Read move from Player %d: %s\n", turn + 1, buffer);

            int row, col;
            bool valid_move = false;

            while (!valid_move) {
                if (sscanf(buffer, "%d %d", &row, &col) != 2 || row < 1 || row > 3 || col < 1 || col > 3) {
                    send_message(current_fd, "Invalid move! Try again. Enter a valid row and column (1-3)");
                    printf("Player %d provided invalid move input: %s\n", turn + 1, buffer);
            
                    // Wait for another input
                    if (Rio_readlineb(current_rio, buffer, MAXLINE) <= 0) {
                        printf("Player %d disconnected during move input.\n", turn + 1);
                        pthread_mutex_unlock(&game_mutex);  // Clean up mutex if needed
                        return NULL;  // Exit the function or handle disconnection
                    }
                    continue;  // Stay in the loop for more input
                }
            
                pthread_mutex_lock(&game_mutex);
                if (board[row - 1][col - 1] != ' ') {
                    send_message(current_fd, "Invalid move! Try again. Position already taken.");
                    printf("Player %d attempted to move to an already occupied position: %d %d\n", turn + 1, row, col);
            
                    pthread_mutex_unlock(&game_mutex);
                    // Wait for another input
                    if (Rio_readlineb(current_rio, buffer, MAXLINE) <= 0) {
                        printf("Player %d disconnected during move input.\n", turn + 1);
                        pthread_mutex_unlock(&game_mutex);  // Clean up mutex if needed
                        return NULL;  // Exit the function or handle disconnection
                    }
                    continue;  // Stay in the loop for more input
                }
            
                // If the move is valid, exit the loop
                valid_move = true;
                pthread_mutex_unlock(&game_mutex);
            }


            // Update board
            board[row - 1][col - 1] = (turn == 0) ? 'X' : 'O';
            winner = check_winner(board);

            // Send updated board to both players
            char board_output[MAXLINE];
            print_board(board, board_output);
            send_message(data->connfd1, board_output);
            send_message(data->connfd2, board_output);
            printf("DEBUG: Sent updated board state to both players:\n%s", board_output);

            pthread_mutex_unlock(&game_mutex);
            turn = 1 - turn;  // Switch turns
        }

        // Announce the result
        if (winner == 'D') {
            send_message(data->connfd1, "Game is a draw!");
            send_message(data->connfd2, "Game is a draw!");
            printf("Game %d ended in a draw.\n", current_game);
        } else {
            int winner_id = (winner == 'X') ? 1 : 2;
            scores[winner_id - 1]++;
            sprintf(buffer, "Player %d wins this game!", winner_id);
            send_message(data->connfd1, buffer);
            send_message(data->connfd2, buffer);
            printf("Player %d won game %d.\n", winner_id, current_game);
        }

        current_game++;
    }

    // Announce final scores
    sprintf(buffer, "Final scores - Player 1: %d, Player 2: %d", scores[0], scores[1]);
    send_message(data->connfd1, buffer);
    send_message(data->connfd2, buffer);
    printf("Game session completed. Final scores - Player 1: %d, Player 2: %d\n", scores[0], scores[1]);

    Close(data->connfd1);
    Close(data->connfd2);
    free(data);
    return NULL;
}


int main(int argc, char **argv) {
    int listenfd, connfd1, connfd2;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    printf("Server is ready to accept connections.\n");

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd1 = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("Player 1 connected.\n");

        connfd2 = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        printf("Player 2 connected.\n");

        game_data *data = malloc(sizeof(game_data));
        data->connfd1 = connfd1;
        data->connfd2 = connfd2;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_game, data);
    }

    return 0;
}






