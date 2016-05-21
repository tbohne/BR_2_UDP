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
#include <openssl/sha.h>  //for sha1
#include <sys/time.h>

#include "Aufgabe2.h"
#include "sender_udp.h"


int main (int argc, char *argv[]) {
	
	int sockfd, err;
	socklen_t length;
	struct sockaddr_in to, from;
	char buff[BUFFERSIZE];
	unsigned short nlength;
	char *name;
	unsigned int filelength;
	size_t bufferlength;     //length of buffer that will be used for sending
	size_t readbytes;        //number of bytes read by fread
	int fd;                  //file descriptor for getting length
	FILE* file;              //file stream
	struct stat filebuf;     //file stats
	unsigned int seqNr;     //number of package to be sent
	char filedatabuff[BUFFERSIZE-5];      //contains data of file
	int i;  //magic number that makes the program work.
	char *shaBuffer;  //holds the complete file across all data packages
	char *shaVal;   //holds the actual sha1-value
	char *shaPtr;

	struct timeval timeout;
        

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
		printf("Socket-Error");
		exit(1);
	}

	// Clearing
	bzero(buff, BUFFERSIZE);

	// To satisfy the MTU of a PPPoE-Connection (max package size)
	if ((bufferlength = nlength + 7) > BUFFERSIZE) {
		printf("Exceeded maximum package size.");
		exit(1);
	}

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
	printf("Length of name = %d\nbuffersize = %zu\nfilesize = %d\n", nlength, strlen(buff), filelength);
	
	printf("Standby for sending..");
	
	//send
	err = sendto(sockfd, buff, bufferlength, 0, (struct sockaddr *)&to, length);
	//handle sending errors
	if (err < 0) {
		printf("sendto-Error");
		exit(1);
	}
	printf("Sent %d bytes\n", err);


	/******* FILE TRANSFER ********/

	//reset sequence number
	seqNr = 0;

	//prepare Sha
	shaBuffer = calloc((size_t)	filelength, 1);
	if(!shaBuffer)
	{
	    printf("Could not allocate shaBuffer\n");
	    return 1;
	}
	shaPtr = shaBuffer;

	
	printf("Commencing file transmission\n");
	do {
	    buff[0] = DATA_T;  //Yes, we're sending data!

	    //put sequence number into next 4 bytes of buffer
	    for(i = 1; i < 5; i++)
	    {
		
		buff[i] = (char) ( (seqNr >> ( (i-1)*8) ) & 0xff );
	    }

	    seqNr++;
	    

	    //check how many files could be read. Is less than BUFFERSIZE-5 when eof is reached
	    readbytes = fread(filedatabuff, 1, BUFFERSIZE-5, file);


	    //transmit if we have any bytes to transmit
	    if(readbytes != 0)
	    {
		//put filebuffer into real buffer. Can't concat because buff is not really a string and can have null anywhere...
		for(i = 5; i<readbytes+5; i++)
		{
		    buff[i] = filedatabuff[i-5];
		    *(shaPtr++)=filedatabuff[i-5];
		}

		//Send data.

		err = sendto(sockfd, buff, readbytes+5, 0, (struct sockaddr *)&to, length);
		if( err != readbytes+5 )
		{
		    printf("Sending data package %d failed",seqNr);
		    return 1;
		}

		printf("Sent package %d containing %zu bytes\n", seqNr, readbytes);
	    }
	}while(readbytes == BUFFERSIZE-5);  //once readbytes is less than BUFFERSIZE-5 eof was reached and we're finished.
	

	printf("File transmission complete\n");


	/******* SHA-1 ********/

	printf("Calculating Sha1...\n");

	//calculate sha-1
	shaVal = getSha1(shaBuffer, filelength);

	//printf("SHA1 of buffer is %s\n", shaVal);	

	//prepare transmitting of sha-1
	buff[0] = SHA1_T;

	for(i = 1; i<SHA_DIGEST_LENGTH*2+1; i++)
	{
	    buff[i] = shaVal[i-1];
	}

	//transmit sha-1
	
	err = sendto(sockfd, buff, SHA_DIGEST_LENGTH*2+1, 0, (struct sockaddr *)&to, length);
	if( err != SHA_DIGEST_LENGTH*2+1 )
	{
	    printf(SHA1_ERROR);
	    return 1;
	}

	printf(SHA1_OK);


	/****** RECEIVE SHA COMPARE RESULT ******/

	timeout.tv_sec = WAIT;
	timeout.tv_usec = 0;
	// Set socket options for a possible timeout
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	//receive sha comp result
	err = recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&from, &length);
	if (err != 2)
	{
	    printf(timeout_error);
	    exit(1);
	}

	//check header
	if(buff[0] != SHA1_CMP_T)
	{
	    printf(SHA1_ERROR);
	}

	//check actual compare result
	if( buff[1]  == SHA1_CMP_ERROR)
	{

	    printf(SHA1_ERROR);
	}
	else
	{
	    printf(SHA1_OK);
	}
	

	/******** CLEAN UP *******/

	
	// Close Socket
	close(sockfd);
	fclose(file);
	free(shaBuffer);
	free(shaVal);
	
	return 0;
}

	    

void prepareHeader(char *buffer, unsigned short nlength, char *name, unsigned int filelength)
{

    unsigned short i,j;
    //unsigned short buffersize;

   	//buffersize = 1;

    
    printf("initialising buffer\n");
    //bzero(buffer, buffersize);
    

    printf("Write header & length\n");
    //TYPE-ID
    buffer[0] = (char) HEADER_T;
    buffer[1] = (char) (nlength & 0xff);
    buffer[2] = (char) ((nlength >> 8) & 0xff);

    printf("Write name\n");
    for(i=3; i<nlength+3; i++)
    {
	buffer[i] = name[i-3];
    }
    printf("Size of unsigned int is %lu\n", sizeof(unsigned int));
    printf("write filelength\n");
    for(j = 0; j < 4; j++)
    {
	buffer[i++] = (char) ( (filelength >> (j*8) ) & 0xff );
	/*printf("Buffer[%d]==%d\n",i,buffer[i]);
	  i++;*/
    }
    //buffer[11] = -32;

    printf("Done.\n");

}
