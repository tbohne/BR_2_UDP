#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "receiver_udp.h"
#include "Aufgabe2.h"

int main (int argc, char *argv[]) {

        int sockfd;  //socket file descriptor
	int length;  //length of address (of to)
	int err;     //return value of bind for error handling
	struct sockaddr_in to, from;  //addresses for receiving and transmitting
	socklen_t fromlen; //length of source address struct

	unsigned char headerstate;

	char buff[1024];  //mesage buffer

	//error handling: argument parsing
	if (argc != 2) {
		printf("Illegal Arguments: [RECEIVER_PORT]");
		exit(1);
	}
	
	// GENERATE SOCKET
	// AF_INET --> Protocol Family
	// SOCK_DGRAM --> Socket Type (UDP)
	// 0 --> Protocol Field of the IP-Header (0, TCP and UDP gets entered automatically)
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0) {
		printf("Socket-Problem");
		exit(1);
	}

	length = sizeof(to);
	// Clearing
	bzero(&to, length);


	//CREATE TARGET ADDRESS
	// Assign Protocol Family
	to.sin_family = AF_INET;
	// Assign Port
	to.sin_port = htons(atoi(argv[1]));
	to.sin_addr.s_addr = inet_addr("127.0.0.1");

	//bind socket to prepare receiving
	err = bind(sockfd, (struct sockaddr *) &to, length);

	if (err < 0) {
		printf("Binding-Problem");
		exit(1);
	}

	// Length of the Source Address Structure
	fromlen = sizeof(struct sockaddr_in);

        

	err = recvfrom(sockfd, buff, 1024, 0, (struct sockaddr *)&from, &fromlen);

	headerstate = (unsigned char) buff[0]+128;
	if(headerstate != HEADER_T)
	{
	    printf("Illegal state: state was %d\n",headerstate);
	    exit(1);
	}
	printf("Received %d bytes\n",err);
	parseHeader(buff);

	
	if (err < 0) {
	  printf("recvfrom-Problem");
	  exit(1);
	}

	printf("Received %d bytes from host %s port %d\n", length, inet_ntoa(from.sin_addr), htons(from.sin_port));

	err = sendto(sockfd, "Message received\n", 17, 0, (struct sockaddr *)&from,fromlen);
	if (err < 0) {
	  printf("sendto-Problem");
	  exit(1);
	}
		
		

	// Close Socket
	close(sockfd);	

	return 0;
}

void parseHeader(char* buffer)
{
    unsigned short i,j;


    unsigned short readnlength;
    char *readname;
    unsigned long readfilelength;

    printf("Reading...\n");
    
    readnlength = ( buffer[1]+128 ) | ( (buffer[2]+128) << 8 );
    printf("name length is %d\n", readnlength);

    readname = calloc(readnlength,1);
    
    for(i = 3; i<readnlength+3; i++)
    {

	readname[i-3] = buffer[i];
    }


    
    for(j = 0; j < readnlength; j++)
    {
	putchar(readname[j]);
    }
    printf("\n");

    free(readname);

    
    /*for(j = i; j < i+4; j++)
    {
	printf("Buffer[%d]==%d\n",j,buffer[j]);
	}*/

    //printf("I is %d\n", i);

    readfilelength = 0;
    for(j = 0; j < 4; j++)
    {
	readfilelength = readfilelength | ( ( buffer[++i]+128)<<(j*8));
	//printf("Buffer[%d]==%d\n",i,buffer[i]);
	//printf("readfilelength = %lu\n", readfilelength);
    }

    printf("file length is %lu\n", readfilelength);



}
