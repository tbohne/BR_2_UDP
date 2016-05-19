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
	unsigned long filelength;
	size_t bufferlength;     //length of buffer that will be used for sending
	size_t readbytes;        //number of bytes read by fread
	int fd;                  //file descriptor for getting length
	FILE* file;              //file stream
	struct stat filebuf;     //file stats
	unsigned long seqNr;     //number of package to be sent
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
		printf("Socket-Problem");
		exit(1);
	}

	// Clearing
	bzero(buff, BUFFERSIZE);

	// To satisfy the MTU of a PPPoE-Connection (max package size)
	if ((bufferlength = nlength + 8) > BUFFERSIZE) {
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

	//reset sequence number
	seqNr = 0;


	//prepare Sha
	shaBuffer = calloc(filelength,1);
	if(!shaBuffer)
	{
	    printf("Could not allocate shaBuffer\n");
	    return 1;
	}
	shaPtr = shaBuffer;

	
	printf("Commencing file transmission\n");
	do {
	    buff[0] = DATA_T-128;  //Yes, we're sending data!

	    //put sequence number into next 4 bytes of buffer
	    for(i = 1; i < 5; i++)
	    {
		
		buff[i] = (char) ( (seqNr >> ( (i-1)*8) ) & 0xff )- 128;
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
		    printf("Sending data package %lu failed",seqNr);
		    return 1;
		}

		printf("Sent package %lu containing %zu bytes\n", seqNr, readbytes);
	    }
	}while(readbytes == BUFFERSIZE-5);  //once readbytes is less than BUFFERSIZE-5 eof was reached and we're finished.
	

	printf("File transmission complete\n");


	/******* SHA-1 ********/

	printf("Calculating Sha1...\n");

	//calculate sha-1
	shaVal = getSha1(shaBuffer, filelength);

	//printf("SHA1 of buffer is %s\n", shaVal);
	

	//prepare transmitting of sha-1
	buff[0] = SHA1_T-128;

	for(i = 1; i<SHA_DIGEST_LENGTH*2+1; i++)
	{
	    buff[i] = shaVal[i-1];
	}

	//transmit sha-1
	
	err = sendto(sockfd, buff, SHA_DIGEST_LENGTH*2+1, 0, (struct sockaddr *)&to, length);
	if( err != SHA_DIGEST_LENGTH*2+1 )
	{
	    printf("Error when sending Sha-1\n");
	    return 1;
	}

	printf("Sha-1 is transmitted.\n");


	/****** RECEIVE SHA COMPARE RESULT ******/


	/************* NEW ******************/

	timeout.tv_sec = WAIT;
	timeout.tv_usec = 0;
	// Set socket options for a possible timeout
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	/***********************************/


	//receive sha comp result
	err = recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&from, &length);
	if (err != 2)
	{
	    printf(timeout_error);
	    exit(1);
	}

	//check header
	if(buff[0]+128 != SHA1_CMP_T)
	{
	    printf("Error when receiving Sha Compare result\n");
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
	

	/******** ALL THE OTHER STUFF *******/

	    
	//await ok-response
	
	//printf("Got an ACK! %s Port: %d\n", inet_ntoa(from.sin_addr), htons(from.sin_port));

	
	
	// Close Socket
	close(sockfd);
	//free(buff);
	fclose(file);
	

	
	free(shaBuffer);
	free(shaVal);
	
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

/*
char* getSha1(char *buff, int bufferlength)
{
    int i = 0;
    unsigned char temp[SHA_DIGEST_LENGTH];
    char *shaBuf;
    char *sha1;

    shaBuf = calloc(SHA_DIGEST_LENGTH*2+1,1);
 
    //bzero(shaBuf, SHA_DIGEST_LENGTH*2);
    bzero(temp, SHA_DIGEST_LENGTH);

    
    //memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
    //memset(temp, 0x0, SHA_DIGEST_LENGTH);

    sha1 = buff;
    
    SHA1((unsigned char *)sha1, bufferlength, temp);
 
    for (i=0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf((char*)&(shaBuf[i*2]), "%02x", temp[i]);
    }
 
    printf("SHA1 of buffer is %s\n", shaBuf);
	
    
    return shaBuf;


}
*/
