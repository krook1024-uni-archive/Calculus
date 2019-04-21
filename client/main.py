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

        self.stacks = []

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

            if len(received) == 0:
                break
                self.closeSocket()
                raise RuntimeError("Socket connection broken!")

            print(received)

            if "surrender" in received:
                print("The other party has surrendered so you win this game!")
                exit(1)


            if "u lost" in received:
                print("You lost this game!")
                self.closeSocket()
                exit(1)

            if "u won" in received:
                print("You won the game! Congratulations!")
                self.closeSocket()
                exit(1)

            if "ur next" in received:
                self.getStats()
                self.printStats()
                (whichOne, howMany) = self.prompt()
                self.sendMsg("take " + str(whichOne) + " " + str(howMany))
                print("Waiting for the other player...")

    def getStats(self):
        self.sendMsg("stats")
        reply = self.receiveMsg()

        # DEBUG
        # print(reply)

        reply = reply.rstrip('\n').split('|')

        del self.stacks
        self.stacks = []

        self.stacks.append(int(reply[0].strip().split(' ')[2]))
        self.stacks.append(int(reply[1].strip().split(' ')[2]))
        self.stacks.append(int(reply[2].strip().split(' ')[2]))

    def printStats(self):
        print("> The first stack has", self.stacks[0], "rocks.")
        print("> The second stack has", self.stacks[1], "rocks.")
        print("> The third stack has", self.stacks[2], "rocks.")

    def resign(self):
        print("You've resigned so you've lost this game!")
        self.sendMsg("resign")
        self.closeSocket()
        exit(0)

    def prompt(self):
        whichOne = howMany = -1

        whichOneIn = input("Which stack do you want to take rocks from? (1-3): ")

        if "feladom" in whichOneIn or "resign" in whichOneIn:
            self.resign()

        howManyIn = input("How many rocks do you want to take? (1-"+str(min(self.maxTakable, self.stacks[whichOne-1]))+"): ")

        if "feladom" in howManyIn or "resign" in howManyIn:
            self.resign()

        try:
            whichOne = int(whichOneIn)
            howMany = int(howManyIn)
        except ValueError:
            return self.prompt()

        if not (1 <= whichOne <= 3):
            return self.prompt()

        if not (1 <= howMany <= self.maxTakable):
            return self.prompt()

        return (whichOne, howMany)

    def sendMsg(self, msg):
        msg = msg.encode()
        sent = self.s.sendall(msg)
        if sent != None:
            raise RuntimeError("socket connection broken")

    def receiveMsg(self):
        # warning, this is buggy.
        # sometimes, not the whole message is received, only a part of it.
        # :(
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
        raise RuntimeError('Invalid IP address given.')

    if serverPort > 65535 or serverPort < 1024:
        raise RuntimeError('Invalid port given (1024-65534)')

    CalcSocket(serverIP, serverPort)
###########################################################################################
# Call main
if __name__ == '__main__':
    main()
