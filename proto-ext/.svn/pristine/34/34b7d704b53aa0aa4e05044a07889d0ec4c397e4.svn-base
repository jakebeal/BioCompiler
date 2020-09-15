# Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
# listed in the AUTHORS file in the MIT Proto distribution's top directory.
# 
# This file is part of MIT Proto, and is distributed under the terms of
# the GNU General Public License, with a linking exception, as described
# in the file LICENSE in the MIT Proto distribution's top directory.

#/ to do: march6 2006
#  network discovery 
#  structural/skeleton discovery???
#  topo -> pc stream motor vals
#  better way to drive topobo from PC? 
#  topobo new functions: 
#    send motor val to PC
#    play motor val from PC
#    send range of motor vals to PC
#    buffer motor vals from PC

# apps: viz and edit motor data on the PC
# sync two devices using a PC and two serial ports



import getopt
import serial
import string
import threading
import Queue
import time
import binascii
import sys
import os
import binascii

# NOTES
# need to play before running motor
# need timeouts and retries on read_packet
# need timeouts

EXITCHARCTER = '\x04'   #ctrl+D

CONVERT_CRLF = 2
CONVERT_CR   = 1
CONVERT_LF   = 0

def post(s):
    sys.stderr.write(s)
    sys.stderr.flush()

def postmsgraw(p):
    for ce in p:
        post(hex(ord(ce)))

def postmsg(p):
    postmsgraw(p)
    post("\n"); 

def reader():
    """loop forever and copy serial->console"""
    is_read = 0
    p = []
    while 1:
        data = s.read()
	if (len(data) > 0):
	    if not is_read:
		post("RX=")
                p = list(data)
            else:
                p = p + list(data)
	    postmsgraw(data)
	    post(" ")
	    is_read = 1
	else:
	    # post("RD %d" % is_read)
	    if is_read:
                q.put(p)
                post("\n")
		is_read = 0
	    # else:
	    #	post(" WAIT\n")
        # sys.stdout.write(data)
        # sys.stdout.flush()

def read_packet ():
    p = q.get(1)
    return p

def read_packet ():
    p = q.get(1)
    return p

def clear_packets ():
    while 1:
        if (q.qsize() > 0):
            p = q.get(0)
        else:
            break
# todo: 2 byte version

def parsepathraw(p):
    r = 0
    for c in p[::-1]: # routing addresses are L to R, buttom up. ie the leaf node has the top 2 bits, the root has the lsb's. 
        r = (r << 2) + (int(c)-1) # int(c)-1 is because channels are 1-4 but bitmasks are 0-3
    for i in xrange(4 - len(p)): # add padding to fill out the byte
        r = r << 2
    return r
        
def parsepath(p):
    if (len(p) == 1 and p[0] == '.'):
        post(" DOT\n")
        return chr(0) + chr(0) + chr(0)
    else:
        return chr(len(p)) + chr(parsepathraw(p)) + chr(0)
        
def query_children(path):
    clear_packets()
    n = 0x22
    post("QUERYING %s \n" % path)
    o = chr(n + 0x80) + parsepath(path)
    s.write(o)
    rx = read_packet()
    if (len(rx) == 4 and rx[0] == chr(0x1b)):
        post("ANSWER %d \n" % ord(rx[3]))
        return ord(rx[3])

def motor(phase, cmd):
    #n = 0x20 # global_position_offset_backpack_message
    n = 0x1d # position_bp_msg 
    post("MOTORING\n")
    if (len(cmd) > 1):
	post("  PATH\n")
	p = parsepath(cmd)
	o = chr(n + 0x80)
    else:
	p = ''
	o = chr(n)
    for i in xrange(0, 256): # motor range
	val = (i + phase)%64
	s.write(o + p + chr(val * 4))
	print i * 4
	sys.stdout.flush()
	time.sleep(0.01)
    post("MOTORNOT\n")

def motorz():
    n = 0x20 # global_position_offset_backpack_message
    post("MOTORING\n")
    o  = chr(n + 0x80)
    p1 = parsepath('.')
    p2 = parsepath('2')
    p3 = parsepath('22')
    for i in xrange(0, 256): # motor range
	val = (i + 0)%64
	s.write(o + p1 + chr(val * 4))
	val = (i + 16)%64
	s.write(o + p2 + chr(val * 4))
	val = (i + 32)%64
	s.write(o + p3 + chr(val * 4))
	time.sleep(0.01)
    post("MOTORNOT\n")


def find_topo(path):
    c  = []
    rx = query_children(path)
    if (rx == -1):
       return c
    else:
	for i in xrange(4):
	    if ((rx >> i) & 1):
		 c = c + [[i+1, find_topo([chr(ord('0') + i+1)] + path)]]
        return c

def writer():
    while 1:
        rcs = sys.stdin.readline()
        cs  = string.split(rcs)
	if (len(cs) > 0):
	    if (cs[0] == 'c'):
		post("CLEAR\n")
		n = 0x04
		o = chr(n)
		s.write(o)
	    elif (cs[0] == 'r'):
		n = 0x02
		post("RECORD\n")
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		postmsg(o)
		s.write(o)
	    elif (cs[0] == 'p'):
		n = 0x03
		post("PLAY\n")
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		postmsg(o)
		s.write(o)
	    elif (cs[0] == 'f'):
		t = find_topo([])
		post("TOPO %s\n" % t)
	    elif (cs[0] == 'i'):
		n = 0x22  # send_id_msg
		post("ID?\n")
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		s.write(o)
	    elif (cs[0] == 'd'):
		n = 0x1d
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		s.write(o)
	    elif (cs[0] == 's'):
		clear_packets()
		n = 0x03 # stop
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		s.write(o)
		v = []
		b = -1
		n = 0x1d
		if (len(cs) > 1):
		    o = chr(n + 0x80) + parsepath(cs[1])
		else:
		    o = chr(n)
		while 1:
		    time.sleep(0.1)
		    # post("WRITING READ REQ\n")
		    s.write(o)
		    # post("READING PCK\n")
		    cs = read_packet()
		    # post("READ PCK\n")
		    if (cs[0] == chr(0x5)):
			post("Got EOS\n")
			break
		    elif (cs[0] == chr(0x23)):
			if (len(cs) == 4):
			    t = ord(cs[1]) * 256 + ord(cs[2])
			    m = ord(cs[3])
			    # post("Raw %d " % ord(cs[1]))
			    # post("%d " % ord(cs[2]))
			    post("Read %d" % m)
			    post(" @ %d\n" % t)
			    # if (t < b):
			    #     break
			    # else:
			    b = t
			    v = v + [[t, m]]
			else:
			    post("Read bogus len packet\n")
		    else:
			post("Read bogus packet\n")
		post("GOT %d PACKETS\n" % len(v))
		time.sleep(0.01)
		s.write(chr(0x04))
		s.write(chr(0x04))
	    elif (cs[0] == 'q'):
		post("QUIT\n")
		break
        elif (cs[0] == 'm'):
	    if (len(cs) > 0):
		cmd = cs[1]
		cmd = ''
	    motor(0, cmd)
        elif (cs[0] == 'q'):
            post("QUIT\n")
            break
        elif (cs[0] == 'M'):
	    post("motoring\n")
            motorz()
            break
        else:
#	    post("unknown\n")
	    #o = binascii.a2b_hex(rcs[:-1]) #strip off the end of line char and convert to hex
	    o = chr(rcs[:-1])
	    post("tx: " + cs + " ")
	    s.write(o)


def getSerialPort ():
    files = os.listdir("/dev")
    match = "cu."
    match2 = "us"
    match3 = "US"
    p = "modem"
    q = "Bluetooth"
    candidates = []

    for f in files:
	if match in f and not p in f and not q in f:
	    if match2 in f or match3 in f:
		candidates.append(f) # add this string to the cadidates list

    print "Assigning serial ports."
    portName = "/dev/"+candidates[0] #use the first serial port
    return portName
    #"/dev/tty.usbserial-151"


if __name__ == '__main__':
    #initialize with defaults
    #port = 0 #works on a PC
    port  = getSerialPort() #works on a mac!
    baudrate = 19200
    echo = 0
    convert_outgoing = CONVERT_CRLF
    rtscts = 0
    xonxoff = 0
    repr_mode = 0
    
    try:
        s = serial.Serial(port, baudrate, bytesize=8,parity='N',stopbits=1, timeout=3) #rtscts=rtscts, xonxoff=xonxoff)
    except:
        post("Could not open port\n")
        sys.exit(1)
    q = Queue.Queue(100)
    v = []
    post("--- Miniterm --- type q to quit\n")
    #start serial->console thread
    r = threading.Thread(target=reader)
    r.setDaemon(1)
    r.start()
    #and enter console->serial loop
    writer()
    post("EXIT\n")
