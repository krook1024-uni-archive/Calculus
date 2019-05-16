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

# This file stores the class CalcSocket for the project 'Calculus'.

import socket
import random
import os

random.seed(a=os.urandom(128))

from fun import *

class CalcSocket:
    def __init__(self, serverIP, serverPort):
        self.msgLen = 256 + 1

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
            instructions()
        except socket.error as err:
            print("Failed to connect! Message: " + str(err))
            exit(1)

    def getRules(self):
        print("Requesting rules from the server...")
        self.sendMsg("rules")
        reply = self.receiveMsg()
        self.maxTakable = int(reply.split('max_takable')[1])
        print("-> You can take", self.maxTakable, "rocks at a time.")

    def mainLoop(self):
        while(True):
            received = self.receiveMsg()

            if len(received) == 0:
                break
                self.closeSocket()
                raise RuntimeError("Socket connection broken!")

            print(received)

            if "SURRENDER" in received:
                print("The other party has surrendered so you win this game!")
                exit(1)


            if "LOST" in received:
                print("You lost this game!")
                self.closeSocket()
                exit(1)

            if "WON" in received:
                print("You won the game! Congratulations!")
                self.closeSocket()
                exit(1)

            if "NEXT" in received:
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

        if "rand" in whichOneIn:
            return (self.randomChoice())

        howManyIn = input("How many rocks do you want to take? (1-"+str(min(self.maxTakable, self.stacks[int(whichOneIn)-1]))+"): ")

        if "feladom" in howManyIn or "resign" in howManyIn:
            self.resign()

        if "rand" in whichOneIn:
            return (self.randomChoice())

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

    def randomChoice(self):
        try:
            availableStacks = []
            for i in range(0,2):
                if self.stacks[i] > 0:
                    availableStacks.append(i)

            randStack = random.randint(1, len(availableStacks))
            randNum = random.randint(1, self.maxTakable)

            if randNum >= self.stacks[randStack-1]:
                print(randNum, randStack, self.stacks[randStack])
                print("You chose to choose randomly! Taking", randNum, "rocks from"
                      " stack", str(randStack) + ".")
                return (randStack, randNum)
            else:
                return (self.randomChoice())
        except ValueError:
            return (self.randomChoice())
        except:
            print("Random choice is not available right now!")
            print("You'll have to input number manually...")
            return (self.prompt())

    def sendMsg(self, msg):
        msg = msg.encode()
        sent = self.s.sendall(msg)
        if sent != None:
            raise RuntimeError("socket connection broken")

    def receiveMsg(self):
        return self.s.recv(self.msgLen).decode()

        """
        chunks = []
        bytes_recd = 0
        while bytes_recd < (self.msgLen - bytes_recd):
            chunk = self.s.recv(self.msgLen)
            if chunk == b'':
                raise RuntimeError("socket connection broken")
            chunks.append(chunk)
            bytes_recd = bytes_recd + len(chunk)

        all_recd = b''.join(chunks)
        return all_recd
        """

    def closeSocket(self):
        print("Exiting now...")
        self.s.close()

