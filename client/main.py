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

###########################################################################################

class CalcSocket:
    def __init__(self, serverIP, serverPort):
        self.serverIP = serverIP
        self.serverPort = serverPort

        self.rocksPerStack = 0
        self.maxTakable = 0

        # Create a socket and connect to it
        self.createSocket()
        self.connectToServer()

        # Get the server rules
        self.getRules()

        # Run the main loop
        self.mainLoop()

        # Close the socket
        self.closeSocket()

    def createSocket(self):
        try:
            self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            print("Socket created...")
        except socket.error as err:
            print("Socket failed to create! Message: " + str(err))
            exit(1)

    def connectToServer(self):
        try:
            self.s.connect((self.serverIP, self.serverPort))
            print("Connected to " + str(self.serverIP) + ":" + str(self.serverPort))
            print(self.receiveMsg())
        except socket.error as err:
            print("Failed to connect! Message: " + str(err))
            exit(1)

    def getRules(self):
        print("Requesting rules from the server...")
        self.sendMsg("rules")
        reply = self.receiveMsg()
        self.maxTakable = int(reply.split('max_takable')[1])
        print("Setting maxTakable to", self.maxTakable)

    def mainLoop(self):
        while(True):
            received = self.receiveMsg()
            print(received)

            if "u lost" in received:
                print("You lost this game!")
                self.closeSocket()
                exit(1)

            if "u won" in received:
                print("You won the game! Congratulations!")
                self.closeSocket()
                exit(1)

            if "ur next" in received:
                self.printStats()
                (whichOne, howMany) = self.prompt()
                self.sendMsg("take " + str(whichOne) + " " + str(howMany))
                self.printStats()

    def printStats(self):
        # TODO: actually parse stats
        self.sendMsg("stats")
        reply = self.receiveMsg().rstrip('\n')
        print(reply)

    def prompt(self):
        whichOne = int(input("Which stack do you want to take rocks from? "))
        howMany = int(input("How many rocks do you want to take? "))

        if not 1 <= whichOne <= 3:
            self.prompt()

        if not 1 <= howMany <= self.maxTakable:
            self.prompt()

        return (whichOne, howMany)

    def sendMsg(self, msg):
        msg = msg.encode()
        sent = self.s.sendall(msg)
        if sent != None:
            raise RuntimeError("socket connection broken")

    def receiveMsg(self):
        return self.s.recv(1024).decode()

    def closeSocket(self):
        print("Exiting now...")
        self.s.close()

###########################################################################################
def usage():
    print("USAGE: client [server IP] [server port]")

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
###########################################################################################
def main():
    argnum = len(sys.argv)

    if argnum < 3:
        usage()
        exit(1)

    serverIP = sys.argv[1]
    serverPort = int(sys.argv[2])

    if not isValidIP(serverIP):
        usage()
        exit(1)

    if serverPort > 65535 or serverPort < 1024:
        usage()
        exit(1)

    CalcSocket(serverIP, serverPort)
###########################################################################################
# Call main
if __name__ == '__main__':
    main()
