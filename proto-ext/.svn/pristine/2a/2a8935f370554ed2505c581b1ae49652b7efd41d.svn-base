#!/pythonw

#
# Josh Lifton (c) 2004
#
# Permission is hereby granted to use and abuse this document
# so long as proper attribution is given.
#
# edited 7Feb2006 by Hayes Raffle to run on Mac os 10.4. It's a little sloppy finding the
# right serial ports, but it seems ok for now. 
#
# NOTE: This file is not part of MIT Proto, but is a separate utility
# bundled with it for convenience.

import Tix
import serial
import string
import threading
import time
import binascii
import os

files = os.listdir("/dev")
match = "cu."
candidates = []

for f in files:
    if match in f:
        candidates.append(f) # add this string to the cadidates list

print "Assigning serial ports."
print "Candidates are:"

if len(candidates) > 1 or not len(candidates):
    print "There are",len(candidates),"serial ports."
else:
    print "No serial port found!"
    
if len(candidates) < 4:
    candidates.append("-")


#remapping the usb-serial converter file to cuaa0
##if len(candidates) > 3:
##    print "Assigning /dev/cuaa0 to",candidates[3]
##
##    cmd1 = "rm /Users/hayes/dev/ttyUSB0"
##    cmd2 = "sudo rm -rf /dev/cuaa0/"
##    cmd3 = "ln -s /dev/"+candidates[3]+" /Users/hayes/dev/ttyUSB0"
##    cmd5 = "sudo ln -s /dev/"+candidates[3]+" /dev/cuaa0"
##
##    print "Exec: ",cmd1
##    os.system(cmd1)
##    print "Exec: ",cmd2
##    os.system(cmd2)
##    print "Exec: ",cmd3
##    os.system(cmd3)
##    print "Exec: ",cmd5
##    os.system(cmd5)


# -- global variables
root = Tix.Tk()
receiveMode = Tix.StringVar(root)
transmitMode = Tix.StringVar(root)
oldReceiveMode = 'Hexadecimal'
oldTransmitMode = 'Hexadecimal'
transmitPacketString = Tix.StringVar(root)
maxPacketSize = Tix.IntVar(root)
packetTimeout = Tix.DoubleVar(root)
portString = Tix.StringVar(root)
baudRateString = Tix.StringVar(root)
packetHeaderString = Tix.StringVar(root)
openOrCloseString = Tix.StringVar(root)

receiveFilters = {
    "ASCII":lambda x: x,
    "Hexadecimal":binascii.b2a_hex,
    "Decimal":lambda x: string.rjust(str(ord(x)),3)
}

transmitFilters = {
    "ASCII":lambda x: x,
    "Hexadecimal":binascii.a2b_hex,
    "Decimal":lambda x: chr(int(x))
}


#need to make a new dictionary to replace portEnumToInt, and replace serial.device() everywhere it exists.
portEnumToInt = {
    candidates[0]:candidates[0],
    candidates[1]:candidates[1],
    candidates[2]:candidates[2],
    candidates[3]:candidates[3]
    
}

# -- global constants
WIDTH = 300
HEIGHT = 500
SERIAL_PORT_CLOSED = 0
SERIAL_PORT_OPENED = 1
CLOSE_SERIAL_PORT = 2
WINDOW_HEIGHT = 40

def configureGUI():
    global packetHistory, maxSizeControl, toSendEntry, baudRateOptionMenu, timeoutControl, receiveOptionMenu
    global transmitOptionMenu, maxSizeControl, packetHeaderEntry, portStringOptionMenu, transmitButton

    root.title('::: MIT Media Lab ::: Responsive Environments Group ::: Serial Terminal :::')

    # -- window setup
    mainFrame = Tix.Frame(root, width=WIDTH, height=HEIGHT)
    mainFrame.bind(sequence='<Destroy>', func=windowDestroyed)
    mainFrame.pack(fill=Tix.BOTH)

    # -- packet history display setup
    packetHistory = Tix.Text(mainFrame, height=WINDOW_HEIGHT)
    packetHistory.config(state=Tix.DISABLED, font='courier')

    # -- transmit packet entry setup
    transmitFrame = Tix.Frame(mainFrame)
    transmitButton = Tix.Button(master=transmitFrame, text="Transmit packet: ", command=sendUserPacket, state=Tix.DISABLED)
    toSendEntry = Tix.Entry(master=transmitFrame, textvariable=transmitPacketString)
    transmitButton.pack(side=Tix.TOP, fill=Tix.X)
    toSendEntry.pack(side=Tix.TOP, fill=Tix.X)

    # -- conrols setup
    controlsFrame = Tix.Frame(mainFrame)

    # -- serial port controls setup
    serialPortControlsLabelFrame = Tix.LabelFrame(controlsFrame, label="Serial Port Settings")
    serialPortControlsFrame = serialPortControlsLabelFrame.frame
    openOrCloseButton = Tix.Button(master=serialPortControlsFrame, text="Open", command=openOrCloseConnection, width=5,
                                    textvariable=openOrCloseString)
    openOrCloseButton.pack(fill=Tix.X, side=Tix.TOP)
    portStringOptionMenu = Tix.OptionMenu(master=serialPortControlsFrame, label="Serial port: ", variable=portString, 
                                          options='label.width 25 label.anchor e menubutton.width 15 menubutton.anchor w')
    for p in range(4):
        portStringOptionMenu.add_command(candidates[p], label=candidates[p])    
    portStringOptionMenu.pack(fill=Tix.X, side=Tix.TOP)
    baudRateOptionMenu = Tix.OptionMenu(master=serialPortControlsFrame, label="Baud rate: ", variable=baudRateString, 
                                          options='label.width 25 label.anchor e menubutton.width 15 menubutton.anchor w')
    ## TODO : The enumeration of baud rates should come from the 'serial' module.
    for r in [300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200]:
        baudRateOptionMenu.add_command(r, label=r)
    baudRateOptionMenu.pack(fill=Tix.X, side=Tix.TOP)
    timeoutControl = Tix.Control(serialPortControlsFrame, label="Packet timeout (seconds): ", variable=packetTimeout,
                                 min=0, inc=lambda x:float(x)+0.001, dec=lambda x:float(x)-0.001)
    timeoutControl.label.config(width=25, anchor='e')
    timeoutControl.pack(fill=Tix.X, side=Tix.TOP)
    serialPortControlsLabelFrame.pack(side=Tix.LEFT)

    # -- packet controls setup
    packetControlsLabelFrame = Tix.LabelFrame(controlsFrame, label="Packet Settings")
    packetControlsFrame = packetControlsLabelFrame.frame
    receiveOptionMenu = Tix.OptionMenu(master=packetControlsFrame, label="Decode receptions as: ", variable=receiveMode, command=updateHeader,
                                       options='label.width 25 label.anchor e menubutton.width 15 menubutton.anchor w')
    for choice in receiveFilters.keys():
        receiveOptionMenu.add_command(choice, label=choice)
    receiveOptionMenu.pack(fill=Tix.X, side=Tix.TOP)
    transmitOptionMenu = Tix.OptionMenu(master=packetControlsFrame, label="Encode transmissions as: ", variable=transmitMode,
                           options='label.width 25 label.anchor e menubutton.width 15 menubutton.anchor w')
    for choice in transmitFilters.keys():
        transmitOptionMenu.add_command(choice, label=choice)
    transmitOptionMenu.pack(fill=Tix.X, side=Tix.TOP)
    packetHeaderEntry = Tix.LabelEntry(packetControlsFrame, label="Look for packet header: ",
          options='label.width 25 label.anchor e entry.width 15 entry.anchor w')
    packetHeaderEntry.entry.config(textvariable=packetHeaderString)
    packetHeaderEntry.pack(fill=Tix.X, side=Tix.TOP)
    maxSizeControl = Tix.Control(packetControlsFrame, label="Max packet size (bytes): ", variable=maxPacketSize, min=1)
    maxSizeControl.label.config(width=25, anchor='e')
    maxSizeControl.pack(fill=Tix.X, side=Tix.TOP)
    packetControlsLabelFrame.pack(side=Tix.RIGHT)

    # -- pack the window up
    packetHistory.pack(fill=Tix.BOTH, side=Tix.TOP)
    transmitFrame.pack(fill=Tix.X, side=Tix.TOP)
    controlsFrame.pack(side=Tix.BOTTOM, fill=Tix.X)
    
    # -- menu bar setup
    mainMenu = Tix.Menu(root)

    # -- file menu
    fileMenu = Tix.Menu(mainMenu,tearoff=0)
    fileMenu.add_separator()
    fileMenu.add_command(label="Quit", command=quit)
    mainMenu.add_cascade(label="File",menu=fileMenu)

    # -- info menu
    infoMenu = Tix.Menu(mainMenu,tearoff=0)
    infoMenu.add_command(label="About...")
    mainMenu.add_cascade(label="Help",menu=infoMenu)

    # -- add menu to frame
    root.config(menu=mainMenu)

    # -- set global variable defaults
    receiveMode.set(oldReceiveMode)
    transmitMode.set(oldTransmitMode)
    maxPacketSize.set(38)
    packetTimeout.set(0.001)
    portString.set(candidates[0])
    baudRateString.set(115200)
    openOrCloseString.set('Open')


def receivePacket(packet):
    if packet != '':
        # Enable the text area, insert the text, reset the insertion point, and disable the text area.
        packetHistory.config(state=Tix.NORMAL)
        packetHistory.insert(Tix.INSERT, 'RX: ' + ASCIIToFormattedString(packet, receiveMode.get()) + '\n')
        packetHistory.mark_set(Tix.INSERT, 0.0)
        packetHistory.config(state=Tix.DISABLED)


def sendUserPacket():
    global serialConnection
    transmitPacketString.set(formatUserInputString(transmitPacketString.get(), transmitMode.get()))
    if (transmitPacketString.get() != ''):
        serialConnection.write(formattedStringToASCII(transmitPacketString.get(), transmitMode.get()))
        packetHistory.config(state=Tix.NORMAL)
        packetHistory.insert(Tix.INSERT, 'TX: ' + transmitPacketString.get() + '\n')
        packetHistory.mark_set(Tix.INSERT, 0.0)
        packetHistory.config(state=Tix.DISABLED)
    

def quit():
    root.quit()


def windowDestroyed(q):
    global stop
    quit()
    stop = 1


def formatUserInputString(input, inputFormat):
    "Returns a version of the input string that conforms to the specified format."
    if inputFormat == "ASCII":
        return input
    elif inputFormat == "Hexadecimal":
        inputList = string.split(input)
        outputList = []
        for s in inputList:
            s = string.zfill(s,len(s) +  len(s)%2)
            for j in range(len(s)/2):
                try:
                    outputList.append(binascii.b2a_hex(binascii.a2b_hex(s[2*j:2*j+2])))
                except TypeError:
                    # -- Some portion of the string is ill-formed, so ignore it.
                    pass
        return string.join(outputList, ' ')
    elif inputFormat == "Decimal":
        inputList = string.split(input)
        outputList = []
        for s in inputList:
            try:
                numValueOfs = int(s)
                if numValueOfs <= 255 and numValueOfs >= 0:
                    outputList.append(str(int(s)))
                else:
                    int('this is just to raise an error')
            except ValueError:
                # -- Some portion of the string is ill-formed, so ignore it.
                pass
        return string.join(outputList, ' ')
    else:
        return ''


def formattedStringToASCII(input, inputFormat):
    "Returns the ASCII version of a preformatted string."
    if inputFormat != 'ASCII':
        input = string.split(input)
        for i in range(len(input)):
            input[i] = apply(transmitFilters[inputFormat], [input[i]])
        input = string.join(input, '')
    return input


def ASCIIToFormattedString(input, outputFormat):
    "Returns a version of the input string in the specified format."
    if outputFormat == 'ASCII':
        return input
    else:
        output = []
        for c in input:
            output.append(apply(receiveFilters[outputFormat], c))
        return string.join(output, ' ')


def convertFormattedString(input, inputFormat, outputFormat):
    "Converts a formatted string from one format to another."
    return ASCIIToFormattedString(formattedStringToASCII(input, inputFormat), outputFormat)


def updateHeader(e):
    global oldReceiveMode
    packetHeaderString.set(convertFormattedString(formatUserInputString(packetHeaderString.get(), oldReceiveMode), oldReceiveMode, receiveMode.get()))
    oldReceiveMode = receiveMode.get()

def openOrCloseConnection():
    global closeSerialPort, serialConnection, serialPortState
    if openOrCloseString.get() == 'Open':
        serialConnection = serial.Serial('/dev/'+portEnumToInt[portString.get()],int(baudRateString.get()), timeout=packetTimeout.get())
        openOrCloseString.set('Close')
        packetHeaderString.set(formatUserInputString(packetHeaderString.get(), receiveMode.get()))
        serialPortState = SERIAL_PORT_OPENED
        transmitButton.config(state=Tix.NORMAL)
        maxSizeControl.config(state=Tix.DISABLED)
        baudRateOptionMenu.config(state=Tix.DISABLED)
        timeoutControl.config(state=Tix.DISABLED)
        receiveOptionMenu.config(state=Tix.DISABLED)
        transmitOptionMenu.config(state=Tix.DISABLED)
        packetHeaderEntry.config(state=Tix.DISABLED)
        portStringOptionMenu.config(state=Tix.DISABLED)
    else:
        serialPortState = CLOSE_SERIAL_PORT
        openOrCloseString.set('Open')
        transmitButton.config(state=Tix.DISABLED)
        maxSizeControl.config(state=Tix.NORMAL)
        baudRateOptionMenu.config(state=Tix.NORMAL)
        timeoutControl.config(state=Tix.NORMAL)
        receiveOptionMenu.config(state=Tix.NORMAL)
        transmitOptionMenu.config(state=Tix.NORMAL)
        packetHeaderEntry.config(state=Tix.NORMAL)
        portStringOptionMenu.config(state=Tix.NORMAL)


def monitorSerialPort():
    global serialPortState, stop, serialConnection
    while not stop:
        # The serial port needs to be closed.
        if serialPortState == CLOSE_SERIAL_PORT:
            serialConnection.close()
            serialPortState = SERIAL_PORT_CLOSED
        # The serial port is open, so read from it.
        elif serialPortState == SERIAL_PORT_OPENED:
            header = packetHeaderString.get()
            if header == '':
                receivePacket(serialConnection.read(maxPacketSize.get()))
            else:
                header = formattedStringToASCII(header, receiveMode.get())
                getRestOfPacket = 1
                for c in header:
                    if serialConnection.read(1) != c:
                        getRestOfPacket = 0
                        break
                if getRestOfPacket:
                    receivePacket(header + serialConnection.read(maxPacketSize.get() - len(header)))
                

def main():
    global serialPortState, stop, serialConnection, packetHeader
    stop = 0
    serialPortState = SERIAL_PORT_CLOSED
    serialConnection = None
    packetHeader = ''
    configureGUI()
    serialPortThread = threading.Thread(target=monitorSerialPort)
    serialPortThread.start()
    try:
        root.mainloop()
    finally:
        stop = 1

main()
