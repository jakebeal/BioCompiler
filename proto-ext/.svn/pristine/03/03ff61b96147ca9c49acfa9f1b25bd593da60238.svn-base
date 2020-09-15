#!/usr/bin/python
# Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
# listed in the AUTHORS file in the MIT Proto distribution's top directory.
# 
# This file is part of MIT Proto, and is distributed under the terms of
# the GNU General Public License, with a linking exception, as described
# in the file LICENSE in the MIT Proto distribution's top directory.

import serial, sys, curses, string
import time
ser = serial.Serial('/dev/tty.KeySerial1', 57600, timeout=None)

#header1 = "" + chr(0x14) + chr(0x2) + chr(0x0) + chr(0xe) + chr(0x0) + chr(0x1)
#script1 = "" + chr(0x1b) + chr(0x0) + chr(0x0) + chr(0x0) + chr(0x1) + chr(0x0) + chr(0x0) + chr(0x1) + chr(0x1) + chr(0x7f) + chr(0x4) + chr(0x3c) + chr(0x0) + chr(0x1)
#header2 = "" + chr(0x09) + chr(0x2) + chr(0x1) + chr(0xe) + chr(0x0) + chr(0x1)
#script2 = "" + 
#header = "" + chr(0x1) + chr(0x7b) + chr(0x0) + chr(0x0)
#script = "" + chr(0xFF)
#msg1 = header1 + script1
#msg2 = header2 + script2
#msgLength1 = len(msg1)
#msgLength2 = len(msg2)
#print len(msg1)
#print len(msg2)
msg1 = "" + chr(0x10) + chr(0x2) + chr(0x0) + chr(0x0) + chr(0xe) + chr(0x0) + chr(0x1) + chr(0x1b) + chr(0x0) + chr(0x0) + chr(0x0) + chr(0x1) + chr(0x0) + chr(0x0) + chr(0x1) + chr(0x1)
msg2 = "" + chr(0xc) + chr(0x2) + chr(0x0) + chr(0x1) + chr(0xe) + chr(0x0) + chr(0x1) + chr(0x81) + chr(0x4) + chr(0x3c) + chr(0x0) + chr(0x1)
while True:
    for i in range(0, 16, 1):
        ser.write(msg1[i])
        time.sleep(0.0002)
    print("half packet out")
    time.sleep(1)
    for i in range(0, 12, 1):
        ser.write(msg2[i])
        time.sleep(0.0002)
    print("1 packet out")
    time.sleep(2.5)
#    for i in range(0, msgLength2, 1):
#        ser.write(msg2[i])
#        time.sleep(0.0002)
#    time.sleep(0.5)
ser.close()
