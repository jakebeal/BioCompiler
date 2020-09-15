#!/usr/bin/python
# Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
# listed in the AUTHORS file in the MIT Proto distribution's top directory.
# 
# This file is part of MIT Proto, and is distributed under the terms of
# the GNU General Public License, with a linking exception, as described
# in the file LICENSE in the MIT Proto distribution's top directory.

import serial, sys, curses, string, socket, select
import time, threading

protoSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
protoSocket.bind(('', 2222))
protoSocket.listen(3)
clientSocket, (remhost, remport) = protoSocket.accept()

class WriterThread (threading.Thread):
    def run ( self ):
        ser = serial.Serial('/dev/tty.KeySerial1', 57600, timeout=None)
        writerQueue = []
        serialTimer = time.time()
        while True:
            [rlist, wlist, elist] = select.select( [clientSocket], [], [] )
            if rlist != []:
                messageLength = "" + clientSocket.recv(1)
                messageContent = "" + clientSocket.recv(ord(messageLength[0]))
                writerQueue.append(messageLength)
                writerQueue.append(messageContent)
            if (len(writerQueue) > 0):
                outgoingMessageLength = ord(writerQueue.pop(0))
                outgoingMessage = writerQueue.pop(0)
                for i in range(0, outgoingMessageLength, 1):
                    ser.write(outgoingMessage[i])
                    time.sleep(0.0002)
                print("Forwarded Message Out!")
                serialTimer = time.time()
                    
                    
class ReaderThread (threading.Thread):
    def run ( self ):
        ser = serial.Serial('/dev/tty.KeySerial1', 57600, timeout=0.5)
        readerQueue = []
        while True:
            [rlist, wlist, elist] = select.select( [], [clientSocket], [] )
            if wlist != [] and len(readerQueue) > 0:
                incomingMessageLength = readerQueue.pop(0)
                incomingMessage = readerQueue.pop(0)
                clientSocket.send(incomingMessageLength)
                clientSocket.send(incomingMessage)
            else:
                messageLength = "" + ser.read(1)
                if len(messageLength) > 0:
                    messageContent = "" + ser.read(ord(messageLength) - 1)
                    if len(messageContent) == (ord(messageLength) - 1):
                        message = messageLength + messageContent
                        readerQueue.append(messageLength)
                        readerQueue.append(message)
                        print("Got message from Protobo.")

                        

ReaderThread().start()
WriterThread().start()
