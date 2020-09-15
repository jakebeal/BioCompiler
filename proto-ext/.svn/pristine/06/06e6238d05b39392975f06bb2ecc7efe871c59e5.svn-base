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
while True:
    x = ser.read()
    print(ord(x), x)

ser.close()
