#!/usr/bin/env python
import os, os.path
import argparse
import socket
import struct
import hashlib
import binascii

SHA1_CMP_OK=0
SHA1_CMP_ERROR=-1

HEADER_T=0
DATA_T=1
SHA1_T=2
SHA1CMP_T = 3
sha1_ok 	= "\x1b[32mSHA1 OK \x1b[0m\n";
sha1_error 	= "\x1b[31mSHA1 Error\x1b[0m\n";
port_error 	= "\x1b[31mInvalid Port (%s) \x1b[0m\n";
address_error 	= "\x1b[31mInvalid Address (%s) or Port (%s) \x1b[0m\n";
receiver_sha1 	= "\x1b[34mReceiver SHA1: %s \x1b[0m\n";
sender_sha1 	= "\x1b[34mSender SHA1: %s \x1b[0m\n";
filename_str 	= "\x1b[33mFilename: %s \x1b[0m\n";
filesize_str 	= "\x1b[33mFilesize: %d bytes\x1b[0m\n";

packet_error = "\x1b[31mInvalid Packet: received %d, expected %d \x1b[0m\n";
order_error = "\x1b[31mInvalid Packet Order: received %d, expected %d \x1b[0m\n";
timeout_error = "\x1b[31mTimeout reached, aborting..\x1b[0m\n";


BUFFERSIZE = 1492
def myrecv(exptype):
    global remote
    try:
        reply,remote = sock.recvfrom(BUFFERSIZE)
    except socket.timeout:
        print timeout_error[:-1]
        exit()
    type_t = struct.unpack("!B",reply[0])[0]
    if type_t != exptype:
        print packet_error[:-1] % (type_t, exptype)
        exit()
    return reply[1:]


def checkfilenamelen(filenamelen,reply):
    if filenamelen > struct.unpack("<H",reply)[0]:
        print "Filename very long or Filenamelength not in NBO"

def checkfilesize(filesize,reply):
    if filesize > struct.unpack("<I",reply)[0]:
        print "Very big file or Filesize not in NBO"


def checkfilename(filename):
    if "/" in filename or "\\" in filename:
        print "Filename contains / or \\, could be a Path"

parser = argparse.ArgumentParser(description='PA2 UDP Test Receiver')
parser.add_argument('port', metavar="Empfaengerport",type=int,help="Port des Empfaenger")
a = parser.parse_args()
if not os.path.isdir("received"):
    print "Folder \"./received\" does not exist..."
    exit()
sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.bind(('',a.port));
remote = ()
reply = myrecv(HEADER_T)
sock.settimeout(10)
filenamelen = struct.unpack("!H",reply[0:2])[0]
checkfilenamelen(filenamelen,reply[0:2])
filename = reply[2:2+filenamelen];
checkfilename(filename)
print filename_str[:-1]%filename
filesize = struct.unpack("!I",reply[2+filenamelen:])[0]
checkfilesize(filesize,reply[2+filenamelen:])
print filesize_str[:-1]%filesize
f = open("received/"+filename,"w+")
sha1 = hashlib.sha1()
seqnr = 0
while(True):
    try:
        reply = myrecv(DATA_T)
    except:
        print timeout_error[:-1]
        sock.shutdown(socket.SHUT_RDWR)
        sock.close()
        exit()

    recseqnr = struct.unpack("!I",reply[0:4])[0]
    if seqnr != recseqnr:
        print order_error[:-1] % (recseqnr, seqnr)
        exit()
    seqnr+=1
    reply = reply[4:]
    f.write(reply)
    sha1.update(reply)
    filesize-=len(reply)
    if not filesize:
        break
reply = myrecv(SHA1_T)
hexsha1 = binascii.hexlify(reply)
print receiver_sha1[:-1] % hexsha1
sha1sum = sha1.digest()
if reply == sha1sum:
    print sha1_ok[:-1]
    buf =""
    buf+= struct.pack("!B",SHA1CMP_T)
    buf+= struct.pack("!B",SHA1_CMP_OK)
    sock.sendto(buf,remote)
else:
    print sha1_error[:-1]
    buf =""
    buf+= struct.pack("!B",SHA1CMP_T)
    buf+= struct.pack("!B",SHA1_CMP_ERROR)
    sock.sendto(buf,remote)
