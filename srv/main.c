/*
			_            _
           | |          | |
   ___ __ _| | ___ _   _| |_   _ ___
  / __/ _` | |/ __| | | | | | | / __|    v0.1.0
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
#include <stdbool.h>
#include <stdarg.h> // concat
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h> // isdigit()

#define MAX_CLIENTS 30
#define MAX_BATTLES 15 // should always be (MAX_CLIENTS/2)

#define MSG_LEN 128

void license(void);
void usage(void);
void printlogo(void);
char *concat(int, ...);
bool str_contains(const char*, const char*);
char *str_pad(char*, int);
char *int_to_string(const int);

int count_connected(void);
void send_server_rules(int, int);
int count_battles(void);
void reset_battle(int, const int);
int get_client_battle_id(int);
bool is_in_battle(int);
int get_other_player_id(int);
void print_battle_stats(int);
void send_battle_stats(int);
int sum_rocks(int);

void disconnect_peer(int);
void send_message(int, char*);
bool on_msg_recv(const int, const char*, int, int);

typedef struct Battle {
	int p1, p2, pNext, stack[3];
} Battle;

// Only global so other functions can access it
int g_client_socket[MAX_CLIENTS];
Battle g_battles[MAX_BATTLES];

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
	char buffer[MSG_LEN + 1];

	fd_set readfs;

	// reset clients just in case there is trash init
	for(int i = 0; i < MAX_CLIENTS; i++) {
		g_client_socket[i] = 0;
	}

	// reset every battle "room"
	for(int i=0; i < MAX_BATTLES; i++) {
		reset_battle(i, rocks_per_stack);
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

		// only accept new connection if we have enough slots
		if(count_connected() < MAX_CLIENTS) {
			// activity on the master socket = new connection
			if(FD_ISSET(master_socket, &readfs)) {
				if((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
					perror("\nError on accept()! Message");
				}

				/*
				// greet new users
				if(send(new_socket, welcome_msg, strlen(welcome_msg), 0) != strlen(welcome_msg)) {
					perror("Error on send()! Message:");
				}
				*/

				int new_socket_id;
				// assign a new id to the new client
				for(int i=0; i < MAX_CLIENTS; i++) {
					if(g_client_socket[i] == 0) {
						g_client_socket[i] = new_socket;
						new_socket_id = i;
						break;
					}
				}

				send_message(new_socket_id, "Welcome to Calculus!");

				printf("New connection from %s:%d (id: %d, socket fd: %d)! Currently online users: %d.\n",
						inet_ntoa(address.sin_addr), ntohs(address.sin_port), new_socket_id, new_socket, count_connected());

				// create a battle for every second player and the player before them
				if((count_connected() % 2) == 0) {
					/*for(int i=0; i < MAX_CLIENTS; i++) {
						printf("g_clienT_socket[%d]: %d\n", i, g_client_socket[i]);
					}*/

					// only if both are still connected
					if(new_socket > 0 && g_client_socket[new_socket_id-1] > 0) {
						int new_battle_id = (count_battles() == 0) ? 0 : count_battles();

						g_battles[new_battle_id].p1 = new_socket_id-1;
						g_battles[new_battle_id].p2 = new_socket_id;
						g_battles[new_battle_id].pNext = g_battles[new_battle_id].p1;

						printf("Creating a battle for %d and %d (battle id: %d)!\n", new_socket_id, new_socket_id-1, new_battle_id);
						print_battle_stats(new_battle_id);

						send_message(g_battles[new_battle_id].pNext, "NEXT");
					}
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

					bool wasInBattle = is_in_battle(i);

					printf("Disconnecting host is currently in a battle so we're resetting "
							"the battle and kicking the other party out!\n");

					disconnect_peer(get_other_player_id(i));
					reset_battle(get_client_battle_id(i), rocks_per_stack);

					close(sd);
					g_client_socket[i] = 0;
					printf("Host disconnected: %s:%d! Currently online users: %d.\n",
							inet_ntoa(address.sin_addr), ntohs(address.sin_port), count_connected());

				}

				else {
					// else read input from client
					buffer[valread] = '\0';

					// call on_msg_recv() which is our handler function for incomming messages
					if(!on_msg_recv(i, buffer, rocks_per_stack, max_takable)) {
						getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
						printf("Error on on_msg_recv()! Client (id: %d): %s:%p\n", sd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
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
	printf("  / __/ _` | |/ __| | | | | | | / __|    v0.1.0\n");
	printf(" | (_| (_| | | (__| |_| | | |_| \\__ \\\n");
	printf("  \\___\\__,_|_|\\___|\\__,_|_|\\__,_|___/\n");
	printf("\n");
}

char*
concat(int count, ...) {
	va_list ap;

	// Find required length to store merged string
	int len = 1; // room for NULL
	va_start(ap, count);
	for(int i=0; i < count; i++)
		len += strlen(va_arg(ap, char*));
	va_end(ap);

	// Allocate memory to concat strings
	char *merged = calloc(sizeof(char),len);
	int null_pos = 0;

	// Actually concatenate strings
	va_start(ap, count);
	for(int i=0; i < count; i++) {
		char *s = va_arg(ap, char*);
		strcpy(merged+null_pos, s);
		null_pos += strlen(s);
	}
	va_end(ap);

	return merged;
}

bool
str_contains(const char *haystack, const char *needle) {
	char *retPtr = strcasestr(haystack, needle);
	if(retPtr != NULL) {
		return 1;
	}
	free(retPtr);
	return 0;
}

char*
str_pad(char *str, int new_len) {
	return str;
}

char*
int_to_string(const int n) {
	int len = snprintf(NULL, 0, "%d", n);
	char *str = malloc(len + 1);
	snprintf(str, len + 1, "%d", n);
	return str;
	free(str);
}

int
count_connected(void) {
	int counter = 0;
	for(int i=0; i < MAX_CLIENTS; i++) {
		if(g_client_socket[i] != 0)
			counter++;
	}
	return counter;
}

void send_server_rules(int client_id, int max_takable) {
	char *str;
	str = concat(2, "max_takable ", int_to_string(max_takable));
	send_message(client_id, str);
	free(str);
}

int
count_battles(void) {
	int counter = 0;
	// we assume it's a battle if the two players init aren't the same
	for(int i=0; i < MAX_BATTLES; i++) {
		if(g_battles[i].p1 != g_battles[i].p2)
			counter++;
	}
	return counter;
}

void
reset_battle(int battleid, const int rocks_per_stack) {
	g_battles[battleid].p1 = 0;
	g_battles[battleid].p2 = 0;
	g_battles[battleid].pNext = 0;

	for(int i=0; i < 3; i++) {
		g_battles[battleid].stack[i] = (int)rocks_per_stack;
	}
}

int
get_client_battle_id(int client_id) {
	int retVal = -1;
	for(int i=0; i < MAX_BATTLES; i++) {
		// check if the user is in a battle AND they are not battling themselves
		if( (g_battles[i].p1 == client_id || g_battles[i].p2 == client_id) && (g_battles[i].p1 != g_battles[i].p2) ) {
			retVal = i;
			break;
		}
	}
	return retVal;
}

bool
is_in_battle(int client_id) {
	return ((get_client_battle_id(client_id)) == -1) ? false : true;
}

int
get_other_player_id(int client_id) {
	int battle_id = get_client_battle_id(client_id);
	return (g_battles[battle_id].p1 == client_id) ?	g_battles[battle_id].p2
												  : g_battles[battle_id].p1;
}

void
print_battle_stats(int battle_id) {
	printf("========[Battle %d]========\n", battle_id);
	printf("- Player 1: %d\n", g_battles[battle_id].p1);
	printf("- Player 2: %d\n", g_battles[battle_id].p2);
	printf("- Next player: %d\n", g_battles[battle_id].pNext);
	for(int i=0; i < 3; i++) {
		printf("- Stack %i: %d\n", i+1, g_battles[battle_id].stack[i]);
	}
	printf("- Sum of rocks: %d\n", sum_rocks(battle_id));
}

void
send_battle_stats(int client_id) {
	int battle_id = get_client_battle_id(client_id);
	int stack1 = g_battles[battle_id].stack[0],
		stack2 = g_battles[battle_id].stack[1],
		stack3 = g_battles[battle_id].stack[2];

	char *str;

	str = concat(9,
				 "stack 1 ", int_to_string(stack1), " | ",
				 "stack 2 ", int_to_string(stack2), " | ",
				 "stack 3 ", int_to_string(stack3), "\n"
				);

	send_message(client_id, str);
	free(str);
}

int
sum_rocks(int battle_id) {
	int retVal = 0;
	for(int i=0; i < 3; i++) {
		retVal += g_battles[battle_id].stack[i];
	}
	return retVal;
}

void
disconnect_peer(int client_id) {
	printf("Forcefully disconnecting peer (id: %d, fd: %d)!\n", client_id, g_client_socket[client_id]);
	close(g_client_socket[client_id]);
	g_client_socket[client_id] = 0;
}

void
send_message(int client_id, char* msg) {
	printf("sending: %s\n", msg);
	if(send(g_client_socket[client_id], msg, strlen(msg), 0) < strlen(msg)) {
		perror("(SendMessage): Error on send()! Message");
	}
}

bool
on_msg_recv(const int client_id, const char* msg, int rocks_per_stack, int max_takable) {
	// dc if user wants to
	if(str_contains(msg, "quit")) {
		disconnect_peer(client_id);
	}

	if(is_in_battle(client_id)) {
		// taking
		int battle_id = get_client_battle_id(client_id);
		if(str_contains(msg, "take")) {
			int whichStack = 0,	howMany = 0;

			// succesful user input
			if(sscanf(msg, "take %d %d", &whichStack, &howMany) == 2) {
				if(whichStack >= 1 || whichStack <= 3) {
					// decrement cause indexes start from 0, not 1
					whichStack--;
					// if stack has enough rocks + user input does not exceed max takable
					if(g_battles[battle_id].stack[whichStack] >= howMany && howMany <= max_takable) {
						// if its actually the senders turn
						if(g_battles[battle_id].pNext == client_id) {
							printf("[Battle %d] Player %d has taken %d rock(s) from stack %d!\n",
								battle_id, client_id, howMany, whichStack);

							// reduce stack by $howMany
							g_battles[battle_id].stack[whichStack] -= howMany;

							if(sum_rocks(battle_id) == 0) {
								send_message(client_id, "WON");

								// determine other player
								int otherPlayer = get_other_player_id(client_id);

								send_message(otherPlayer, "LOST");

								printf("Player (id: %d) won against Player (id: %d). Exiting now!\n",
										client_id, otherPlayer);
								sleep(3); exit(0);
							} else {
								// set pNext to other playa
								g_battles[battle_id].pNext = (g_battles[battle_id].pNext == g_battles[battle_id].p1)
									? g_battles[battle_id].p2 : g_battles[battle_id].p1;

								send_message(g_battles[battle_id].pNext, "NEXT");
							}

							print_battle_stats(battle_id);
						}
					}
				}
			}
		}

		// surrender
		if(str_contains(msg, "resign") || str_contains(msg, "feladom")) {
			printf("Client (id: %d, fd: %d) surrendered so we are disconnecting them and their partner (battle id: %d)!\n",
					client_id, g_client_socket[client_id], get_client_battle_id(client_id));
			int battle_id = get_client_battle_id(client_id);
			send_message(get_other_player_id(client_id), "SURRENDER");
			disconnect_peer(g_battles[battle_id].p1);
			disconnect_peer(g_battles[battle_id].p2);
			reset_battle(battle_id, rocks_per_stack);
		}

		if(str_contains(msg, "stats")) {
			send_battle_stats(client_id);
		}
	}

	if(str_contains(msg, "rules")) {
		send_server_rules(client_id, max_takable);
	}

	return true;
}
