# Compiler and Flags
CC = gcc
CFLAGS = -Wall -pthread -g

# Source Files
SERVER_SRC = server.c csapp.c
CLIENT_SRC = client.c csapp.c
CSAPP_SRC = csapp.c
CSAPP_HEADER = csapp.h

# Executable Names
SERVER_EXEC = server
CLIENT_EXEC = client

# Build Targets
all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRC) $(CSAPP_HEADER)
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC)

$(CLIENT_EXEC): $(CLIENT_SRC) $(CSAPP_HEADER)
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC)

# Clean up build files
clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
