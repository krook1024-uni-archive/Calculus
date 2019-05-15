#!/usr/bin/env python3
#            _            _
#           | |          | |
#   ___ __ _| | ___ _   _| |_   _ ___
#  / __/ _` | |/ __| | | | | | | / __|    v0.1.0
# | (_| (_| | | (__| |_| | | |_| \__ \
#  \___\__,_|_|\___|\__,_|_|\__,_|___/
#
#
#    Calculus - a simple terminal-based game based on socket programming
#    Copyright (C) 2019 Molnár Antal Albert
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

# This file stores functions for the project 'Calculus'.

import socket

def isValidIP(address):
    try:
        socket.inet_pton(socket.AF_INET, address)
    except AttributeError:
        try:
            socket.inet_aotn(address)
        except socket.error:
            return False
        return address.count('.') == 3
    except socket.error:
        return False

    return True

def usage():
    print("USAGE: client [server IP] [server port]")
