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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H
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
#include "Battle.h"

extern int g_client_socket[MAX_CLIENTS];
extern Battle g_battles[MAX_BATTLES];

void license(void);
void usage(void);
void printlogo(void);
char *concat(int, ...);
bool str_contains(const char*, const char*);
char *str_pad(char*, int);
// char *int_to_string(const int); //

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

#endif
