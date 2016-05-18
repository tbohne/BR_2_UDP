#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>  //for file stats
#include <unistd.h>    //for checking file access
#include <libgen.h>    //for getting file name


#include "Aufgabe2.h"
#include "sender_udp.h"



int main (int argc, char *argv[]) {
	
	int sockfd, err;
	socklen_t length;
	struct sockaddr_in to, from;
	char buff[BUFFERSIZE];
	unsigned short nlength;
	char *name;
	unsigned long filelength;
	size_t bufferlength;     //length of buffer that will be used for sending
	size_t readbytes;        //number of bytes read by fread
	int fd;                  //file descriptor for getting length
	FILE* file;              //file stream
	struct stat filebuf;     //file stats
	unsigned long seqNr;     //number of package to be sent
	char* filedatabuff[BUFFERSIZE-5];      //contains data of file
	int i;
	
	
	//char* buff;

	/****** CHECK INPUT ********/

	
	//check for right number of arguments
	if (argc != 4) {
		printf("Illegal Arguments: [RECEIVER_ADDRESS] [RECEIVER_PORT] [FILE PATH]");
		exit(1);
	}

	//check if file exists and can be read
	if( access(argv[3], R_OK) == -1 )
	{
	    if( access(argv[3], F_OK) )
	    {
		printf("File does not exist");
	    }
	    else
	    {
		printf("No Read permission on file");
	    }
	    return 1;
	}


	/*******  OPEN FILE OPERATIONS *******/


	//Open file, handle errors
	file = fopen(argv[3], "r");
        
	if(!file)
	{
	    printf("Illegal File");
	    return 1;
	}


	//Get file descriptor
	fd = fileno(file);
	if(fd == 0)
	{
	    printf("File reading error");
	    return 1;
	}

	//Get file stats
	if(fstat(fd, &filebuf) != 0)
	{
	    printf("File statistic read error");
	    return 1;
	}




	/**** READ FILE STATS ****/
	//get length
	filelength = filebuf.st_size;

	//get file name without path
	name = basename(argv[3]);

	nlength = strlen(name);
		



	
        





	
	/******** SOCKET CREATION ***********/
	// AF_INET --> Protocol Family
	// SOCK_DGRAM --> Socket Type (UDP)
	// 0 --> Protocol Field of the IP-Header (0, TCP and UDP gets entered automatically)
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0) {
		printf("Socket-Problem");
		exit(1);
	}

	// Clearing
	bzero(buff, BUFFERSIZE);
	bufferlength = nlength + 8;

//	buff = calloc(bufferlength, 1);
	
	//CREATE TARGET ADDRESS
	// Assign Protocol Family
	to.sin_family = AF_INET;
	// Assign Port
	to.sin_port = htons(atoi(argv[2]));
	// Length of the Address Structure
	length = sizeof(struct sockaddr_in);
	// Address of the Receiver (Dotted to Network)
	to.sin_addr.s_addr = inet_addr(argv[1]);
	

	



	/********* HEADER SENDING *********/
	prepareHeader(buff, nlength, name, filelength);
	printf("Length of name = %d\nbuffersize = %zu\nfilesize = %lu\n", nlength, strlen(buff), filelength);
	
	printf("Standby for sending..");


	
	//send
	err = sendto(sockfd, buff, bufferlength, 0, (struct sockaddr *)&to, length);
	//handle sending errors
	if (err < 0) {
		printf("sendto-Problem");
		exit(1);
	}
	printf("Sent %d bytes\n", err);





	/******* FILE TRANSFER ********/
	seqNr = 0;

	do {
	    buff[0] = DATA_T;
	    for(i = 0; i < 4; i++)
	    {
		
		buff[i] = (char) ( (seqNr >> ( (i-1)*8) ) & 0xff )- 128;
	    }

	    readbytes = fread(filedatabuff, BUFFERSIZE-5, 1, file);
	    
	    err = sendto(sockfd, buff, readbytes+5, 0, (struct sockaddr *)&to, length);
	

/*
	       if(readbytes == 0)
	    {
		printf("File empty");
		
		}*/
	    printf("Sent package %lu containing %zu bytes", seqNr, readbytes);
	}while(readbytes == BUFFERSIZE);
	





	/******** ALL THE OTHER STUFF *******/

	    
	//await ok-response
	err = recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&from, &length);

	
	
	if (err < 0) {
		printf("recvfrom-Problem");
		exit(1);
	}

	printf("Got an ACK! %s Port: %d\n", inet_ntoa(from.sin_addr), htons(from.sin_port));

	
	
	// Close Socket
	close(sockfd);
	//free(buff);
	fclose(file);
	
	return 0;
}

	    

void prepareHeader(char *buffer, unsigned short nlength, char *name, unsigned long filelength)
{

    unsigned short i,j;
    unsigned short buffersize;

    buffersize = 8 + nlength;

    
    printf("initialising buffer\n");
    bzero(buffer, buffersize);
    

    printf("Write header & length\n");
    //TYPE-ID
    buffer[0] = (char) HEADER_T-128;
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

    

    
    printf("Done.\n");
  

}
