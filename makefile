CFLAGS = -Wall -std=c99 -pedantic -D_POSIX_SOURCE -g -lssl -lcrypto
PORT = 25565
ADDRESS = 127.0.0.1
#192.168.239.128
FILEPATH = files/sample
SRCPATH = src/
BINPATH = bin/
VALGPARAMS = --leak-check=full --track-origins=yes

build: received $(BINPATH)sender_udp $(BINPATH)receiver_udp

debug: clearconsole build

$(BINPATH)receiver_udp: $(SRCPATH)receiver_udp.c $(SRCPATH)receiver_udp.h
	gcc $(CFLAGS) -o $(BINPATH)receiver_udp $(SRCPATH)receiver_udp.c

$(BINPATH)sender_udp: $(SRCPATH)sender_udp.c $(SRCPATH)sender_udp.h
	gcc $(CFLAGS) -o $(BINPATH)sender_udp $(SRCPATH)sender_udp.c	

received:
	mkdir received

clearconsole:
	reset

clean:
	rm bin/* received/*

testrec: build
	$(BINPATH)receiver_udp $(PORT)

testsend: build
	$(BINPATH)sender_udp $(ADDRESS) $(PORT) $(FILEPATH)

valgrec: build
	valgrind $(VALGPARAMS) $(BINPATH)receiver_udp $(PORT)

valgsend: build
	valgrind $(VALGPARAMS) $(BINPATH)sender_udp $(ADDRESS) $(PORT) $(FILEPATH)

