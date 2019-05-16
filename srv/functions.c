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

#include "functions.h"

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
	char *pad = "#";

	int len = (int)strlen(str);
	if(len >= new_len) {
		return str;
	}

	for(int i=0; i < (new_len - len); i++) {
		strcat(str, pad);
	}

	return str;
}

/* BROKEN --- Causes heap leakage.
char*
int_to_string(const int n) {
	int len = snprintf(NULL, 0, "%d", n);
	char *str = (char*)malloc(sizeof(char) * len + 1);
	snprintf(str, len + 1, "%d", n);
	return str;
}
*/

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

	// convert max_takable to string
	char s_max_takable[20];
	sprintf(s_max_takable, "%d", max_takable);

	str = concat(2, "max_takable ", s_max_takable);
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

	char s_stack[3][20];
	sprintf(s_stack[0], "%d", stack1);
	sprintf(s_stack[1], "%d", stack2);
	sprintf(s_stack[2], "%d", stack3);

	str = concat(9,
				 "stack 1 ", s_stack[0], " | ",
				 "stack 2 ", s_stack[1], " | ",
				 "stack 3 ", s_stack[2], "\n"
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
	/*char buf[MSG_LEN + 1];
	strcpy(msg, buf);
	printf(buf, str_pad(buf, MSG_LEN));*/

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
								battle_id, client_id, howMany, whichStack + 1);

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
