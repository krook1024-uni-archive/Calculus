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

#include "config.h"
#include "functions.h"
#include "Battle.h"

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

	int port = atoi(*(++argv));
	int rocks_per_stack = atoi (*(++argv));
	int max_takable = atoi (*(++argv));

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
	int master_socket, addrlen, new_socket, activity, valread, sd, max_sd;
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
