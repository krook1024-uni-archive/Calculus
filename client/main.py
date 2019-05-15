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

import sys
import socket
import time as t

from fun import *
from CalcSocket import CalcSocket

def usage():
    print("USAGE: client [server IP] [server port]")

def main():
    argnum = len(sys.argv)

    if argnum < 3:
        usage()
        exit(1)

    try:
        serverIP = sys.argv[1]
        serverPort = int(sys.argv[2])
    except:
        usage()
        exit(1)

    if not isValidIP(serverIP):
        raise RuntimeError('Invalid IP address given.')

    if serverPort > 65535 or serverPort < 1024:
        raise RuntimeError('Invalid port given (1024-65534)')

    CalcSocket(serverIP, serverPort)

# Call main
if __name__ == '__main__':
    main()
