CFLAGS = -Wall -std=gnu99 -pedantic
PORT = 25565
ADDRESS = 127.0.0.1
#192.168.239.128
FILEPATH = files/testfile
SRCPATH = src/
BINPATH = bin/

build: $(BINPATH)sender_udp $(BINPATH)receiver_udp

debug: clearconsole build

$(BINPATH)receiver_udp: $(SRCPATH)receiver_udp.c
	gcc $(CFLAGS) -o $(BINPATH)receiver_udp $(SRCPATH)receiver_udp.c

$(BINPATH)sender_udp: $(SRCPATH)sender_udp.c
	gcc $(CFLAGS) -o $(BINPATH)sender_udp $(SRCPATH)sender_udp.c	

clearconsole:
	reset

clean:
	rm bin/*

testrec: build
	$(BINPATH)receiver_udp $(PORT)

testsend: build
	$(BINPATH)sender_udp $(ADDRESS) $(PORT) $(FILEPATH)

valgrec: build
	valgrind $(BINPATH)receiver_udp $(PORT)

valgsend: build
	valgrind $(BINPATH)sender_udp $(ADDRESS) $(PORT) $(FILEPATH)

