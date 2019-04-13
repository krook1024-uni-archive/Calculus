/*
		    _            _
           | |          | |
   ___ __ _| | ___ _   _| |_   _ ___
  / __/ _` | |/ __| | | | | | | / __|    v0.0.0
 | (_| (_| | | (__| |_| | | |_| \__ \
  \___\__,_|_|\___|\__,_|_|\__,_|___/


   	Calculus - a simple terminal-based game based on socket programming
    Copyright (C) 2019 Molnár Antal Albert

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#define _GNU_SOURCE // strcasestr()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_CLIENTS 30

void license(void);
void usage(void);
void printlogo(void);
char *concat(const char*, const char*);
int strContains(const char*, const char*);
int onMessageReceived(const int, const char*);

// Only global so other functions can access it
int g_client_socket[MAX_CLIENTS];

typedef struct Player {
	int id = 0;

} Player;

typedef struct Stack {
	int num;
} Stack;

typedef struct Battle {
	int p1, p2;
} Battle;

int
main(int argc, char **argv) {
	printlogo();
	license();

	if(argc != 1 + 3) {
		usage();
		exit(1);
	}

	unsigned short int port = atoi(*(++argv));
	unsigned short int rocks_per_stack = atoi (*(++argv));
	unsigned short int max_takable = atoi (*(++argv));

	if(port < 1024 || port > 65535) {
		usage();
		exit(1);
	}

	if(rocks_per_stack < 1 || rocks_per_stack > 10000) {
		usage();
		exit(1);
	}

	if(max_takable < 1 || max_takable > rocks_per_stack) {
		usage();
		exit(1);
	}

	printf("Rules on this server:\n");
	printf("  - There will be %d rocks per stack.\n", rocks_per_stack);
	printf("  - The maximum a player can take at once is %d rocks.\n", max_takable);

	int opt = 1;
	unsigned short int master_socket, addrlen, new_socket, activity, valread, sd, max_sd;
	struct sockaddr_in address;
	char *welcome_msg = "Welcome to Calculus!\n";
	char buffer[1024 + 1];

	fd_set readfs;

	for(int i = 0; i < MAX_CLIENTS; i++) {
		g_client_socket[i] = 0;
	}

	// master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("\nsocket() failed! Message");
		exit(1);
	}

	// set master socket to allow multiple conns
	if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
		perror("\nsetstockopt() failed! Message");
		exit(1);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// bind to $port
	if(bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind() failed! Message");
		exit(1);
	}

	printf("\nStarting server on 0.0.0.0:%d...\n", port);

	// max 3 pending connections
	if(listen(master_socket, 3) < 0) {
		perror("listen");
		exit(1);
	}

	addrlen = sizeof(address);
	puts("Waiting for connections...");

	for(;;) {
		// clear the socket set
		FD_ZERO(&readfs);

		// add master socket to the set
		FD_SET(master_socket, &readfs);
		max_sd = master_socket;

		for(int i = 0; i < MAX_CLIENTS; i++) {
			// socket descriptor
			sd = g_client_socket[i];

			// if valid socket descriptor then add to read list
			if(sd > 0) FD_SET(sd, &readfs);

			// highest file descriptor number, need it for select()
			if(sd > max_sd) max_sd = sd;
		}

		// wait for activity (timeout is NULL so wait until it happens)
		activity = select(max_sd + 1, &readfs, NULL, NULL, NULL);

		if((activity < 0) && (errno != EINTR)) printf("\nError on select()!\n");

		// activity on the master socket = new connection
		if(FD_ISSET(master_socket, &readfs)) {
			if((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
				perror("\nError on accept()! Message");
			}

			printf("New connection from %s:%d (socket fd: %d)!\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), new_socket);

			// greet new users
			if(send(new_socket, welcome_msg, strlen(welcome_msg), 0) != strlen(welcome_msg)) {
				perror("Error on send()! Message:");
			}

			// assign a new id to the new client
			for(int i=0; i < MAX_CLIENTS; i++) {
				if(g_client_socket[i] == 0) {
					g_client_socket[i] = new_socket;
					break;
				}
			}

		}

		// if its not on the master socket then its a received msg or smth
		for(int i=0; i < MAX_CLIENTS; i++) {
			sd = g_client_socket[i];

			if(FD_ISSET(sd, &readfs)) {
				// if some1 wants to disconnect
				if((valread = read(sd, buffer, 1024)) == 0) {
					getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
					printf("Host disconnected: %s:%d!\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					close(sd);
					g_client_socket[i] = 0;
				}

				else {
					// else read input from client
					buffer[valread] = '\0';

					// call onMessageReceived() which is our handler function for incomming messages
					if(onMessageReceived(sd, buffer) != 0) {
						getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
						printf("Error on onMessageReceived()! Client (id: %d): %s:%p\n", sd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					}
				}
			}
		}
	}

	return 0;
}

void
license(void) {
    printf("Calculus    Copyright (C) 2019 Molnár Antal Albert\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions.\n\n");
}

void
usage(void) {
	printf("USAGE: server [PORT] [#_OF_ROCKS] [MAX_TAKABLE]\n\n");
	printf("  PORT is the port you wish to be running the server on.\t(1024-65534)\n");
	printf("  #_OF_ROCKS is the number of rocks per stack\t(1-10000)\n");
	printf("  MAX_TAKABLE defines the maximum number of rocks a player can take at once\t(1-#_OF_ROCKS)\n");
	printf("\nEXAMPLE: server 23145 12 4\n");
	printf("\t\n");
}

void
printlogo(void) {
	printf("            _            _\n");
	printf("           | |          | |\n");
	printf("   ___ __ _| | ___ _   _| |_   _ ___\n");
	printf("  / __/ _` | |/ __| | | | | | | / __|    v0.0.0\n");
	printf(" | (_| (_| | | (__| |_| | | |_| \\__ \\\n");
	printf("  \\___\\__,_|_|\\___|\\__,_|_|\\__,_|___/\n");
	printf("\n");
}

char
*concat(const char *s1, const char *s2) {
	char *result = malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(result, s1);
	strcat(result, s2);
	return result;
	free(result);
}

int
strContains(const char *haystack, const char *needle) {
	char *retPtr = strcasestr(haystack, needle);
	if(retPtr != NULL) {
		return 1;
	}
	free(retPtr);
	return 0;
}

int
onMessageReceived(const int client_id, const char* msg) {
	// echo everything back for now
	char *reply = concat("Your message was: ", msg);
	send(client_id, reply, strlen(reply), 0);
	free(reply);

	return 0;
}
