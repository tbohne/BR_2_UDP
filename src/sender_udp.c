#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "Aufgabe2.h"
#include "sender_udp.h"



int main (int argc, char *argv[]) {
	
	int sockfd, err;
	socklen_t length;
	struct sockaddr_in to, from;
	//char buff[BUFFERSIZE];
	unsigned short nlength;
	char *name;
	unsigned long filelength;
	size_t bufferlength;
	char* buff;


	//fill dummy values
	name = "Es war einmal mitten im Winter, und die Schneeflocken fielen wie Federn vom Himmel herab. Da saß eine Königin an einem Fenster, das einen Rahmen von schwarzem Ebenholz hatte, und nähte.";
	nlength = strlen(name);
	filelength = 128;

	
	if (argc != 4) {
		printf("Illegal Arguments: [RECEIVER_ADDRESS] [RECEIVER_PORT] [FILE PATH]");
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

	// Clearing
	//bzero(buff, BUFFERSIZE);
	buff = calloc(nlength + 7, 1);

	//CREATE TARGET ADDRESS
	// Assign Protocol Family
	to.sin_family = AF_INET;
	// Assign Port
	to.sin_port = htons(atoi(argv[2]));
	// Length of the Address Structure
	length = sizeof(struct sockaddr_in);
	// Address of the Receiver (Dotted to Network)
	to.sin_addr.s_addr = inet_addr(argv[1]);
	

	
	prepareHeader(buff, nlength, name, filelength);

	printf("Standby for sending..");
	bufferlength = nlength + 7;
	
	//send
	err = sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr *)&to, length);
	//handle sending errors
	if (err < 0) {
		printf("sendto-Problem");
		exit(1);
	}




	//await ok-response
	err = recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&from, &length);

	if (err < 0) {
		printf("recvfrom-Problem");
		exit(1);
	}

	printf("Got an ACK! %s Port: %d: %s", inet_ntoa(from.sin_addr), htons(from.sin_port), buff);

	
	
	// Close Socket*/
	close(sockfd);
	free(buff);
	
	return 0;
}




void prepareHeader(char *buffer, unsigned short nlength, char *name, unsigned long filelength)
{

    unsigned short i,j;
    unsigned short buffersize;

    buffersize = 7 + nlength;

    
    printf("initialising buffer\n");
    bzero(buffer, buffersize);
    

    printf("Write header & length\n");
    //TYPE-ID
    buffer[0] = (char) HEADER_T;
    buffer[1] = (char) (nlength & 0xff)-128;
    buffer[2] = (char) ((nlength >> 8) & 0xff)-128;

    printf("Write name\n");
    for(i=3; i<nlength+3; i++)
    {
	buffer[i] = name[i-3];
    }
    

    printf("write filelength\n");

    for(j = 0; j < 4; j++)
    {
	buffer[++i] = (char) ( (filelength >> (j*8) ) & 0xff )- 128;
    }
  
    printf("Done.");
  

}
