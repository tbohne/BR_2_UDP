CFLAGS = -c -Wall

build: sender receiver

receiver: 
	gcc $(CFLAGS) -o ./bin/receiver_udp ./src/receiver_udp.c

sender:
	gcc $(CFLAGS) -o ./bin/sender_udp ./src/sender_udp.c	

clean: 
	rm bin/*