#!/opt/local/bin/python
# Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
# listed in the AUTHORS file in the MIT Proto distribution's top directory.
#
# This file is part of MIT Proto, and is distributed under the terms of
# the GNU General Public License, with a linking exception, as described
# in the file LICENSE in the MIT Proto distribution's top directory.

#/ this program loads a file into flash on the topobo board
# user must enter the filename to bootload when executing this script
# 'python bootload.py myfile.hex'

# basically, i stripped out all the portable and flexible object oriented
# stuff to find the topobo board and do it manually here.
# file uploads work at 38400 baud, not faster. 

import sys, serial, os, IntelHexFormat, AvrProgram, time

files = os.listdir("/dev")
match = "cu."
match2 = "us"
match3 = "US"
match4 = "Serial"
p = "modem"
q = "Bluetooth"
candidates = []

for f in files:
    if match in f and not p in f and not q in f:
        if match2 in f or match3 in f or match4 in f:
            candidates.append(f) # add this string to the cadidates list

#print "Assigning serial ports."

#if len(candidates) > 1 or not len(candidates):
#    print "There are",len(candidates),"serial ports."
#else:
#    print "No serial port found!"
    

# edit this if you need to:
#portName = "/dev/tty.usbserial-351"
portName = "/dev/"+candidates[0] #use the first serial port
#"/dev/tty.usbserial-151"
baudrate = "38400"
mega32filename = 'topobo_serial_gateway.hex'
mega644filename = 'topobo_serial_gateway.hex'

# read the filename off the command line if you want to.
#if len(sys.argv) !=2:
#        print "usage: python %s <filename>"%(sys.argv[0])
#        sys.exit(-1)
#filename = sys.argv[1]
#print "file to upload is " + filename 


# open the serial port.
# print "Opening port "+portName+"..."
try:
    serialconn = serial.Serial(portName,baudrate,bytesize=8,parity='N',stopbits=1, timeout=3)
    #print "port opened"   
except:
    print "couldn't open port. exiting now..."
    sys.exit(-1)


# Parse a IHX file and upload it to the bootloader.
serialconnection = serialconn
if serialconnection:

    deviceType = AvrProgram.findAVRBoard (serialconnection)

    # get the filename
    if deviceType == 114 : #ATmega32
	filename = mega32filename
    elif deviceType == 150 : #ATmega644 
	filename = mega644filename
    else :
	print 'bad device type'
	sys.exit(-1)

    # parse the hex file & load it
    print 'Uploading firmware.... '
            
    ihx = IntelHexFormat.IntelHexFile(filename)    

    try:
	AvrProgram.doFlashProgramming(serialconn, ihx.toByteString(), deviceType)
	print 'Firmware Upload Complete.'

    except serial.SerialException, e:
	controller.updateStatusText('Programming failed: ' + e.value)
                                
else:
    sys.exit(-1)

