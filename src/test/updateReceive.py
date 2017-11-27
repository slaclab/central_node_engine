#!/usr/bin/env python

import socket
import sys
import os
import argparse
import time

#
# Application update buffer offsets
# Header
# [ -64 -   -1] Unknow 64-bit junk (!)
# [   0 -   64] timestamp
# [  64 -  127] 0x0000_0000_0000_0000_0000
# [ 128 -  511] App 0 status
# [ 512 -  896] App 1 status
# [ 897 - 1281] App 2 status
# [    ...    ] ... goes until App 1023

# * WRONG
# Input line:
#
# Input 0 ... Input 1
# bit bit ...
#  0   1  ...
# --- --- ...
# X X X X ...
# | | | |
# | | | + wasHigh
# | | +- wasLow
# | |
# | + wasHigh
# +- wasLow
#
def readFile(f, debug):
    appData = bytearray()

    lineData = bytearray([0, 0, 0, 0, 0, 0, 0, 0, # 32 bits for junk
                          0, 0, 0, 0, 0, 0, 0, 0, # 32 bits for junk
                          0, 0, 0, 0, 0, 0, 0, 0, # 32 bits for timestamp
                          0, 0, 0, 0, 0, 0, 0, 0, # 32 bits for timestamp
                          0, 0, 0, 0, 0, 0, 0, 0, # Zeroes
                          0, 0, 0, 0, 0, 0, 0, 0]) # Zeroes

    appData = appData + lineData
    appCount = 0
    highLine = True # The first line has the wasHigh values, the second line has the wasLow values
    numBits = 192    

    for line in f:
        if debug and highLine:
            print "+---------------------------------------+"
            print "| Global AppId #" + str(appCount) + " "
            print "+---------------------------------------+"

        lineData = bytearray([0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0])
        
        current = 0
        byteIndex = 0
        end = len(lineData)

        line.rstrip()

        bitIndex = 0 # points to which char we are in the line
        inputCount = 1
        
        while current < end:
            debugOut = ""
            byte = 0
            bitIndex = 0 # bit index within input byte

            # For each input read wasLow/wasHigh and set proper bits
            # One byte has bits for 4 inputs
            debugOut = "| " + str(inputCount).zfill(3) + ".." + str(inputCount+3).zfill(3) + "\t| "

            for i in range(8): # write out one byte
                bitValue = 0

                # Read wasLow/wasHigh for input
                if current < len(line):
                    if line[current] == '1':
                        bitValue = 1
                    current = current + 1

                byte |= (bitValue << bitIndex)
                bitIndex = bitIndex + 1

                inputCount = inputCount + 1

            # end for
            lineData[byteIndex] = byte
            byteIndex = byteIndex + 1

        # end while

        appData = appData + lineData
        if highLine == False:
          appCount = appCount + 1
        highLine = not highLine
    # end for line
    return appData

def sendUpdate(sock, file_base, index, host, port, debug):
    file_name = file_base + "-" + str(index) + '.txt'
    if not os.path.isfile(file_name):
        print "ERROR: file " + file_name + " can't be opened, please check if it exists"
        sys.exit()

    f = open(file_name, 'r')

    appdata = readFile(f, debug)

    NUM_APPLICATIONS = 1024
    APPLICATION_SIZE = 512/8 # 64 bytes per application

    try:
        sock.sendto(appdata, (host, port))
        print 'Update #' + str(index) + " sent. (file=" + file_name + ")"

    except socket.error, msg:
        print 'Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        sys.exit()

    f.close()

def readMitigationFile(f, debug):
    appData = bytearray()

    line = f.readline()
    for x in line.strip().split(' '):
        appData.append(int(x))

    return appData
    
def receiveMitigation(sock, mitigation_base, index, host, port, debug):
    file_name = mitigation_base + "-" + str(index) + '.txt'
    if not os.path.isfile(file_name):
        print "ERROR: file " + file_name + " can't be opened, please check if it exists"
        sys.exit()

    f = open(file_name, 'r')

    expectedMitigation = readMitigationFile(f, debug)

    try:
        data, addr = sock.recvfrom(8)

    except sock.error, msg:
        print 'Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        sys.exit()

    a = bytearray(data)
    mitIndex = 0;
    mitigation = bytearray()
    for x in a:
        mitigation.append(x & 0xF)
        mitigation.append((x >> 4) & 0xF) 

    if mitigation != expectedMitigation:
        i = 0
        for m, e in zip(mitigation, expectedMitigation):
            i = i + 1
            if m != e:
                print "ERROR: expected power class " + str(e) + " for mitigation device #" + str(i) + ", got " + str(m) + " instead."

    f.close()

parser = argparse.ArgumentParser(description='Send link node update to central_node_engine server')
parser.add_argument('--host', metavar='hostname', type=str, nargs=1, help='Central node hostname')
parser.add_argument('input', metavar='input_name', type=str, nargs=1, help='input file')
parser.add_argument('mitigation', metavar='mitigation_name', type=str, nargs=1, help='expected mitigation file')
parser.add_argument('--start', metavar='start_index', type=int, nargs='?', help='start index (default=1)')
parser.add_argument('--size', metavar='size', type=int, nargs='?', help='number of input files (default=1)')
parser.add_argument('--port', metavar='size', type=int, nargs='?', help='server port (default=4356)')
parser.add_argument('--debug', action="store_true", help='enable debug output')
parser.add_argument('--repeat', metavar='number', type=int, nargs='?', help='repeat a finite number of times (default=0 -> forever)')

args = parser.parse_args()

 # create dgram udp socket
try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except socket.error:
    print 'Failed to create socket'
    sys.exit()
 
if args.host:
    host = args.host[0]
else:    
    host = 'lcls-dev3'

port=4356
if args.port:
    port = args.port

debug = False
if args.debug:
    debug = True

repeat = 0
forever = True
if args.repeat:
    repeat = args.repeat
    forever = False

file_index = 1
if args.start:
    file_index = args.start

num_files = 1
if args.size:
    num_files = args.size

file_base = args.input[0]
mitigation_base = args.mitigation[0]

if forever:
    msg = "Sending " + str(num_files) + " updates to central node engine at " + host + ":" + str(port) + ". Press ctrl-C to stop."
else:
    msg = "Sending " + str(num_files) + " updates to central node engine at " + host + ":" + str(port) + ". Repeating " + str(repeat) + " times."
print msg


repeatCounter = 0;
done = False
while not done:
    i = file_index
    repeatCounter = repeatCounter + 1
    print "Update cycle #" + str(repeatCounter)
    while i < file_index + num_files:
        sendUpdate(sock, file_base, i, host, port, debug)
        receiveMitigation(sock, mitigation_base, i, host, port, debug)
        i = i + 1
    if not forever:
        if repeatCounter == repeat:
            done = True
