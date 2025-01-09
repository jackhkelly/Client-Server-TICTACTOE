// Final Project - Jack Kelly - Tic Tac Toe
#include "csapp.h"
#include <string.h>
#include <stdbool.h>

// Determines if a message is a board line
bool is_board_line(const char *line) {
    while (*line) {
        if (*line != '.' && *line != 'X' && *line != 'O' && *line != ' ' && *line != '\n') {
            return false;
        }
        line++;
    }
    return true;
}

// Handles messages from the server and sends appropriate responses
void handle_server_message(const char *message, rio_t *rio, int clientfd) {
    char buffer[MAXLINE];
    char last_line[MAXLINE];

    // Print the received message
    printf("%s", message);

    // Loop to process additional lines (for multi-line board updates)
    while (is_board_line(message)) {
        printf("%s", message);  // Print the board row
        if (Rio_readlineb(rio, buffer, MAXLINE) == 0) {
            break;  // Exit if no more data
        }
        message = buffer;  // Update the message to the next line
    }

    // Extract the last meaningful line from the server's message
    strncpy(last_line, message, MAXLINE);
    last_line[MAXLINE - 1] = '\0';  // Ensure null-terminated string

    // Handle specific prompts or server messages based on the last line
    if (strstr(last_line, "Enter the number of games") != NULL) {
        // Prompt the user for the number of games and send it to the server
        printf("Enter the number of games: ");
        Fgets(buffer, MAXLINE, stdin);  // Get input from the user
        Rio_writen(clientfd, buffer, strlen(buffer));  // Send the input to the server
    } else if (strstr(last_line, "Your turn!") != NULL || strstr(last_line, "Your turn, Player") != NULL) {
        // Prompt the user for their move and send it to the server
        printf("Enter your move (row column): ");
        Fgets(buffer, MAXLINE, stdin);  // Get the player's move
        Rio_writen(clientfd, buffer, strlen(buffer));  // Send the move to the server
    } else if (strstr(last_line, "Invalid move! Try again.") != NULL) {
        // Handle invalid move and prompt again
        printf("Invalid move! Try again.\n");
        printf("Enter your move (row column): ");
        Fgets(buffer, MAXLINE, stdin);  // Get a valid move
        Rio_writen(clientfd, buffer, strlen(buffer));  // Send the move to the server
    } else if (strstr(last_line, "Waiting for the other player") != NULL) {
        // Notify the user to wait
        printf("Waiting for the other player to make a move...\n");
    } else if (strstr(last_line, "Game is a draw") != NULL) {
        // Notify the user about a draw
        printf("Game ended in a draw!\n");
    } else if (strstr(last_line, "wins this game") != NULL) {
        // Notify the user about the game winner
        printf("A player has won this game!\n");
    } else if (strstr(last_line, "Final scores") != NULL) {
        // Display the final scores
        printf("Final scores received.\n");
    }
}

int main(int argc, char **argv) {
    int clientfd;
    char *host, *port;
    char buffer[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (1) {
        if (Rio_readlineb(&rio, buffer, MAXLINE) == 0) {  // Connection closed
            printf("Connection closed by server.\n");
            break;
        }

        // Process the server's message and handle cases where input is required
        handle_server_message(buffer, &rio, clientfd);
    }

    Close(clientfd);
    return 0;
}






