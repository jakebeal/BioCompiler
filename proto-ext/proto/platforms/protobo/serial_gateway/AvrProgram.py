#/
# An AVR programmer that uses the AVR-PROG protocol + some speeded up xfers
# (used most commonly in bootloaders!)
# For more info go to: http://sourceforge.net/projects/javrprog/
# JAvrProg is licensed under GPLv2.0
#
# Copyright (C) 2005 Limor Fried and Michael Broxton
#
# Adapted from the JAvrProg project by Michael Broxton

#
# Useful Constants 
#
ATMEGA162_FLASH_SIZE = 16384
ATMEGA162_EEPROM_SIZE = 512
ATMEGA162_FLASHPAGE_SIZE = 128
ATMEGA162_EEPROMPAGE_SIZE = 4

ATMEGA32_FLASH_SIZE = 32768
ATMEGA32_EEPROM_SIZE = 1024
ATMEGA32_FLASHPAGE_SIZE = 128
ATMEGA32_EEPROMPAGE_SIZE = 4

ATMEGA644_FLASH_SIZE = 65536
ATMEGA644_EEPROM_SIZE = 2048
ATMEGA644_FLASHPAGE_SIZE = 256
ATMEGA644_EEPROMPAGE_SIZE = 8

#
# BASIC UTILITIES
#
import serial, time, sys, curses

def setAddress(serialconn, addr):
    addr = addr / 2
    serialconn.write('A')
    serialconn.write(chr((addr >> 8) & 0xff))
    serialconn.write(chr(addr & 0xff))
        
    # print 'setting addr: ' + str(addr);
    
    serialconn.setTimeout(0.1)
    b =  serialconn.read()
    if b != chr(0x0d):
        raise AVRException,'Bad response to setAddress message'
	print AVRException,'Bad response to setAddress message'
            
def eraseFlash(serialconn):
        
    setLED(serialconn)
    serialconn.write('e')
    
    serialconn.setTimeout(3.0)
    b =  serialconn.read()
    
    if b != chr(0x0d):
        raise AVRException,'Bad response to erase message'
	print  AVRException,'Bad response to erase message'
    clearLED(serialconn)
  
def setLED(serialconn):
    serialconn.write('x')
    serialconn.write(chr(0x00));
    
    serialconn.setTimeout(0.1)
    b =  serialconn.read()
    if b != chr(0x0d):
        raise AVRException,'Bad response to setLED message'
        print AVRException,'Bad response to setLED message'
        
def clearLED(serialconn):
    serialconn.write('y');
    serialconn.write(chr(0x00));
    
    serialconn.setTimeout(0.1)
    b =  serialconn.read()
    if b != chr(0x0d):
        raise AVRException,'Bad response to setLED message'
        
  
def readFuseH(serialconn):
    serialconn.write('N')
    
    serialconn.setTimeout(0.1)
    b =  serialconn.read()
    if len(b) != 0:
        return ord(b)        
    else:
        raise AVRException,'No AVR detected on the serial port'
    
def readFuseL(serialconn):
    serialconn.write('F')
    
    serialconn.setTimeout(0.1)
    b =  serialconn.read()
    if len(b) != 0:
        return ord(b)
    else:
        raise AVRException,'No AVR detected on the serial port'


#
# Returns: An integer composed of two bytes from
# the serial port.
#
def readWord(address, serialconn):
    setAddress(serialconn, address)
    serialconn.write('R');
    try:
        serialconn.setTimeout(1.0)
        b =  serialconn.read(2)

        ret = (ord(b[0]) << 8) + ord(b[1])
        return ret;

    except serial.SerialException, e:
        raise AVRException,'Bad response to readWord message: ' + e.value

  
  
def programFlash(buffer, startaddr, endaddr, serialconn, deviceType):

    serialconn.setTimeout(0.1)

    if deviceType == 150:
        flashsize = ATMEGA644_FLASH_SIZE
        flashpagesize = ATMEGA644_FLASHPAGE_SIZE
    elif deviceType == 114:
        flashsize = ATMEGA32_FLASH_SIZE
        flashpagesize = ATMEGA32_FLASHPAGE_SIZE
        
    #    flashsize = device.getFlashSize()
    #    flashpagesize = device.getFlashPageSize()

    bootloaderAddr = 0
    usercodeAddr = 0

    #print 'Endaddr is ' + str(endaddr)
    print 'Endaddr is ' + hex(endaddr).upper()

    if ((startaddr % flashpagesize != 0) | (endaddr % flashpagesize != 0)):
        raise AVRException, 'Start or end address does not line up with page sizes'

    #    progressbar.setValue(0, 'Writing Address: 0x00');

    # print 'Writing ' + (endaddr - startaddr) + ' bytes of Flash to ' + device.name()
    setLED(serialconn)
                                    
    #
    # First we check to make sure that autoincrementing works
    #
    serialconn.write('a')
    b = serialconn.read()
    if b != 'Y':
        raise AVRException,'Bad response..want autoincrementing!' 
        
    #
    # Enter programming mode
    #
    serialconn.write('P')
    b = serialconn.read();
    if b != chr(0x0D):
        raise AVRException,'Bad response in programFlash to P message'
    
    setAddress(serialconn, startaddr)

    fastmode = True; 
    # print 'Writing Hex'         
    i = startaddr - flashpagesize
    row = (endaddr - startaddr) / flashpagesize
    prog = progressBar(0, row, 79) #testing...
    row = 0

    while i < endaddr:
	i += flashpagesize
        #print 'Writing Address ' + hex(i).upper()
                    
	#update the progress bar
	row += 1
	prog.updateAmount(row)
	print prog,'\r',
	sys.stdout.flush()
                  
        # Read the next page into buffer
        
        #        print arraycopy(buffer, "+i+", flashpagebuffer, 0, "+flashpagesize+"); #  fixme for debugging
        flashpagebuffer = buffer[i:i+flashpagesize]
                
        #
        # Check if this next page
        # is all 0xFF and therefore blank, then skip it if it is
        #                         
        blankpage = True;
        
        for j in range(0, flashpagesize):
            if flashpagebuffer[j] !=  chr(0xFF):
                blankpage = False
                break
                
        if (blankpage):
            # print 'Skipping " + str(i)
            setAddress(serialconn, i+flashpagesize)
            continue
            
                                      
        # if fastmode, do it fast!
        if (fastmode):
            #                tryagain:    ???
            try:
                serialconn.write('Z')
                                          
                if i == startaddr:	
                    ret = '!'
                    try:
                        ret = serialconn.read();
                    except serial.SerialException, e:
                        raise AVRException,'Serial exception in response to Z message: ' + e.value
                        
                    # System.out.println("Fast?");
                    if ret == '?':
                        fastmode = False
                        #
                        # this is kinda icky, but basically, undo the incrementation & start over
                        #
                        i -= flashpagesize
                        continue
                                   
                    #print 'Using block write mode'
                        
                # serialconn.send_slow(flashpagebuffer, flashpagesize);
                serialconn.write(flashpagebuffer)
                b = serialconn.read()
                if b != chr(0x0D):
		    raise AVRException,'Bad response during fast write'
                continue
            except serial.SerialException, e:
                # try again?
                print 'Serial exception during write: ' + e.value
                print 'Restarting from address ' +  hex(i).upper() #str(i)
                                          
                for n in range(0,flashpagesize+5):
                    serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                try:
                    serialconn.setTimeout(1.0)
                    serialconn.read()     # MEME - this used to be serealconn.readbytes() in java!!!
                    serialconn.setTimeout(0.1)
                except serial.SerialException, e:
                    raise AVRException,'Serial exception in response to restart message: ' + e.value
                setAddress(serialconn, i)
                i -= flashpagesize   # (to undo the forloop increment) - doesn't work in python!
                serialconn.write('E') # erase page
                serialconn.read()    # read 0x0D
                continue

        else:
            #
            # ok slow mode, send one byte at a time :(
            #
            try:
                for j in range(0,flashpagesize,2):
                    serialconn.write('c')
                    serialconn.write(flashpagebuffer[j])
                    b = serialconn.read()
                    if b != char(0x0D):
                        raise AVRException,'Bad response to c message in slow mode'
                    serialconn.write('C');
                    serialconn.write(flashpagebuffer[j+1]);
                    
                    for q in range(0,5):
                        b = serialconn.read()
                        if b != chr(0x3F): # !!! FIXME: why does this happen??
                            break
                            
                        if b != chr(0x0D):
                            raise AVRException, 'Bad response to C message in slow mode'
            except serial.SerialException, e:
                # hmm, we didnt get a response, lets try again
                print 'No response, restarting from address ' +  hex(i).upper() #str(i)
                serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                serialconn.write(chr(0x1B))
                try:
                    serialconn.setTimeout(1.0)
                    serialconn.read()     # MEME - this used to be serealconn.readbytes() in java!!!
                    serialconn.setTimeout(0.1)
                except serial.SerialException, e:
                    raise AVRException,'Serial exception in response to restart message: ' + e.value
                
                setAddress(serialconn, i)
                i -= flashpagesize   # (to undo the forloop increment)
                continue

            # stupid autoincrement means we have to reset the address!
            setAddress(serialconn, i)
            serialconn.write('m')
                                        
            if b != chr(0x0D):
                raise AVRException, 'Bad response to m message'
                
            setAddress(serialconn, i+flashpagesize)

    #    progressbar.setValue(100, 'Upload Complete')
    print '\r\nUpload Complete'
    clearLED(serialconn)
                                    
    # leave programming mode
    serialconn.write('L')
    b = serialconn.read();
    if b != chr(0x0D):
        raise AVRException, 'Bad response to m message'


def findAVRBoard(serialconnection):

    print 'Attempting to locate Topobo on port ' + serialconnection.portstr

    #
    # Make several attempts to establish a connection to the
    # bootloader on the AVR.
    #
    for i in range(0,3):
        serialconnection.write(chr(0x1B))
        serialconnection.write(chr(0x1B))
        serialconnection.write(chr(0x1B))
        serialconnection.write(chr(0x1B))
        serialconnection.write('S')
        try:
            serialconnection.setTimeout(0.500)
            recvdata = serialconnection.read(100)  # Read up to 100 bytes before the timeout fires
        except: #serial.SerialException, e:
            print 'Serial exception occured in findAVRBoard: ' + e.value
        if recvdata != '':
            break
  
    if (recvdata == '') or (len(recvdata) != 7):
        # print 'Length: ' + str(len(recvdata))
        print 'Failed.'
        raise AVRException, 'Topobo did not respond. Make sure Topobo is in Bootload mode.'
        return False
    
    # Else...
    print 'Found ' + recvdata

    #
    # Get supported devices
    #
    serialconnection.write('t')
    try:
        serialconnection.setTimeout(0.1)
        recvdata = serialconnection.read(500) # Read up to 100 bytes before the timeout fires
    except serial.SerialException, e:
        print 'Serial exception occured in findAVRBoard: ' + e.value
        return False

    if recvdata[-1] != chr(0):
        return False
    
    # print 'supported: '

    for i in range(0,len(recvdata)-1):
        #          DeviceDescriptor d = (DeviceDescriptor)DevicesByID.get(new Byte(recvdata[i]));
        deviceType = ord(recvdata[i])
        if deviceType == 114 :
            mcu = 'ATmega32'
        elif deviceType == 150 :
            mcu = 'ATmega644'
        else :
            mcu = 'unknown!!!'
        # print 'Orginal device ID value: ' + str(deviceType), 
        print 'MCU=' + mcu + '... ', 
            
    return deviceType;


def doFlashProgramming(serialconnection, hexfilebuffer, deviceType):
    #
    # get the address from the fuses
    #
    highfuse = readFuseH(serialconnection)
    highfuse >>= 1
    highfuse &= 3

    if deviceType == 150 : #using mega644, device id value: 150
        bootloadAddr = 0x7e00*2 #in words, not bytes
        flashPageSize = ATMEGA644_FLASHPAGE_SIZE


    elif deviceType == 114 : #using mega32, device id value: 114
        if highfuse == 1:
            bootloadAddr = 0x3C00*2 #1e00
        elif highfuse == 2:
            bootloadAddr = 0x3E00*2 #0x1F00
        elif highfuse == 3:
            bootloadAddr = 0x3f00*2 #0x1F80 
        else:
            bootloadAddr = 0x3800*2 # 0x1C00
        
        flashPageSize = ATMEGA32_FLASHPAGE_SIZE
    
    else:
        print "device type not known. Sorry!"
        sys.exit(-1) 
    
    print 'Bootloader is at addr ' + hex(bootloadAddr).upper(),
    #print hex(bootloadAddr).upper()

    #
    # Erase the flash memory
    #
    eraseFlash(serialconnection)

    #
    # Program the chip!
    #
    programSize = (bootloadAddr - flashPageSize) #program the full memory range

    # length = len(hexfilebuffer)    
    # print 'length :' + str(length)
    # make sure the program isn't too long
    # if (length > (bootloadAddr)) :
    #    print "your program is too big! Better luck next time."
    #    sys.exit(1)

    try : #program the full memory range
        programFlash(hexfilebuffer, 0, programSize, serialconnection, deviceType)

    except serial.SerialException, e:
        print "oops. no programming!"

class AVRException(serial.SerialException):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)


class progressBar:
	def __init__(self, minValue = 0, maxValue = 10, totalWidth=12):
		self.progBar = "[]"   # This holds the progress bar string
		self.min = minValue
		self.max = maxValue
		self.span = maxValue - minValue
		self.width = totalWidth
		self.amount = 0       # When amount == max, we are 100% done 
		self.updateAmount(0)  # Build progress bar string
	
	def updateAmount(self, newAmount = 0):
		if newAmount < self.min: newAmount = self.min
		if newAmount > self.max: newAmount = self.max
		self.amount = newAmount
		
		# Figure out the new percent done, round to an integer
		diffFromMin = float(self.amount - self.min)
		percentDone = (diffFromMin / float(self.span)) * 100.0
		percentDone = round(percentDone)
		percentDone = int(percentDone)
		
		# Figure out how many hash bars the percentage should be
		allFull = self.width - 2
		numHashes = (percentDone / 100.0) * allFull
		numHashes = int(round(numHashes))
		
		# build a progress bar with hashes and spaces
		self.progBar = " [" + '#'*numHashes + ' '*(allFull-numHashes) + "]"
		
		# figure out where to put the percentage, roughly centered
		percentPlace = (len(self.progBar) / 2) - len(str(percentDone)) 
		percentString = " " + str(percentDone) + "%" + " "
		
		# slice the percentage into the bar
		self.progBar = self.progBar[0:percentPlace] + percentString + self.progBar[percentPlace+len(percentString):]
	
	def __str__(self):
		return str(self.progBar)
