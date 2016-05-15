CFLAGS = -Wall
PORT = 25565
ADDRESS = 192.168.239.128
FILEPATH = dummy

build: sender receiver

debug: clearconsole build

receiver: 
	gcc $(CFLAGS) -o ./bin/receiver_udp ./src/receiver_udp.c

sender:
	gcc $(CFLAGS) -o ./bin/sender_udp ./src/sender_udp.c	

clearconsole:
	reset

clean:
	rm bin/*

testrec: build
	bin/receiver_udp $(PORT)

testsend: build
	bin/sender_udp $(ADDRESS) $(PORT) $(FILEPATH)
