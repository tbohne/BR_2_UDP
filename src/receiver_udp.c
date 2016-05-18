#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "receiver_udp.h"
#include "Aufgabe2.h"



int main (int argc, char *argv[]) {

        int sockfd;  //socket file descriptor
	int length;  //length of address (of to)
	int err;     //return value of bind for error handling
	struct sockaddr_in to, from;  //addresses for receiving and transmitting
	socklen_t fromlen; //length of source address struct
	unsigned char state; //state of receiving: 1: expecting Header, 2: expecting Data or end of data, 3: expecting SHA. Just for double-checking.

	unsigned char headerstate;

	char buff[BUFFERSIZE];  //mesage buffer
	
	unsigned short filenamelength;  //length of file name
	char *filename;  //name of file
	unsigned long filelength;  //length of file in bytes
	unsigned long receivedBytes; //Number of received data files (excluding header of each package!)

	struct stat folderstat = {0};  // to check if received folder exists
	char filepath[MAXPATHLENGTH];  //path to file in received folder with file name
	FILE* file;  //File stream to write file into
	unsigned long seqNr;  //number of packet received in data transmission
	unsigned long readSeqNr;  //sequence number transmitted by sender
	char *filebuffer; //holds the file content to write on file
	
	




	
        /****** CHECK INPUT ********/

	
	//error handling: argument parsing
	if (argc != 2) {
		printf("Illegal Arguments: [RECEIVER_PORT]");
		exit(1);
	}
	
	




	
	/******** SOCKET CREATION ***********/
	
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


	/****** RECEIVE HEAD *******/
	state = HEADER_T;
	

	err = recvfrom(sockfd, buff, BUFFERSIZE, 0, (struct sockaddr *)&from, &fromlen);

	headerstate = (unsigned char) buff[0]+128;
	if(headerstate != state)
	{
	    printf("Illegal state: state was %d\n",headerstate);
	    exit(1);
	}
	printf("Received %d bytes\n",err);


	//bzero(filename, filenamelength);  //otherwhise might be used uninitialised
	//filelength = 0;
	parseHeader(buff, &filenamelength, &filename, &filelength);

	
	if (err < 0) {
	  printf("recvfrom-Problem");
	  exit(1);
	}

	printf("Received %d bytes from host %s port %d\n", length, inet_ntoa(from.sin_addr), htons(from.sin_port));


	printf("Parsed file length: %lu\nParsed filename: %s\n", filelength, filename);



	

	/*******  OPEN FILE OPERATIONS *******/


	printf("Preparing file for writing...\n");

	//filename should not exceed limits. If it is too long there could be bufferoverflow, security issues, ...
	if(strlen(filename) + 10 > MAXPATHLENGTH)
	{
	    printf("Can not create path: file name too long");
	    return 1;
	}
	  

	
	//get file path
	snprintf(filepath, MAXPATHLENGTH, "%s%s", "received/", filename);
        


	
	

	//check if folder exists, create if it doesn't
	
	if (!( stat("/received", &folderstat) == 0) && (S_ISDIR(folderstat.st_mode)) ){
	    if (0!= mkdir("/received", 0700))
	    {
		printf("error when creating directory\n");
		perror("mkdir");
		return 1;
	    }
	}



	
	
	//Open file, handle errors
        printf("Open file path %s\n", filepath);
	file = fopen(filepath, "w");
	if(!file)
	{
	    printf("Illegal File");
	    return 1;
	}


	

	/******* READ FILE TRANSMISSION *********/

	//progress state
	state = DATA_T;

	seqNr = 0;
	receivedBytes = 0;

	/*filebuffer = calloc(BUFFERSIZE - 5, 1); //get space for filebuffer
	if (filebuffer == NULL)
	{
	    printf("Could not allocate memory for filebuffer");
	    return 1;
	    }*/


	
	printf("Standing by for incoming file...\n");
	

	//start receiving and interpreting transmission
	do {
	    err = recvfrom(sockfd, buff, BUFFERSIZE, 0, (struct sockaddr *)&from, &fromlen);


	    //read header state
            headerstate = (unsigned char) buff[0]+128;
	    if(headerstate != state)
	    {
		printf("Illegal state: state was %d\n",headerstate);
		exit(1);
	    }

            //read sequence number
	    readSeqNr =  (unsigned long) (  ( buff[1]+128 ) | ( (buff[2]+128) << 8 ) | ( (buff[3]+128) << 16 ) | ( (buff[4]+128) << 24 ) );
	    if(seqNr != readSeqNr)
	    {
		printf("Packets out of order\n");
		return 1;
	    }

	    //set filebuffer to where the real buffer starts
	    filebuffer = buff+5;


	    
	    printf("Received %d payload bytes in packet %lu, writing now\n",err-5, seqNr);

	    //write current package to hard drive.
	    //Note: Either we write often here or we allocate the complete filesize as memory and write to hard drive once finished.
	    //I decided to to the former. It causes latency but is better for large files.
	    fwrite(filebuffer, 1, err-5, file);

	    seqNr++;
	    receivedBytes += err - 5;  //received bytes must only store the number of bytes belonging to the data, not the head of the package

	}while(receivedBytes != filelength); //once we have received everything we're done

	printf("File written on drive.\n");
	

	
	/******* OTHER STUFF ******/
	

	err = sendto(sockfd, "Message received\n", 17, 0, (struct sockaddr *)&from,fromlen);
	if (err < 0) {
	  printf("sendto-Problem");
	  exit(1);
	}
		
		

	// Close Socket
	close(sockfd);
	fclose(file);
	free (filename);
	
	return 0;
}




void parseHeader(char* buffer, unsigned short *readnlength, char **readrealname, unsigned long *readfilelength)
{
    unsigned short i,j;
    
    char *readname;

    //printf("Reading...\n");

    *readnlength = (unsigned short) (  ( buffer[1]+128 ) | ( (buffer[2]+128) << 8 ) );
    //printf("name length is %d\n", *readnlength);

   
    readname = calloc(*readnlength+1,1);

    
    //read name:
    for(i = 3; i<*readnlength+3; i++)
    {
	readname[i-3] = buffer[i];
    }



    
    /*for(j = 0; j < *readnlength; j++)
    {
	putchar(readname[j]);
	}*/
    //printf("\n");

    *readrealname = readname;
    
    /*for(j = i; j < i+4; j++)
    {
	printf("Buffer[%d]==%d\n",j,buffer[j]);
	}*/

    //printf("I is %d\n", i);

    *readfilelength = 0;
    for(j = 0; j < 4; j++)
    {
	*readfilelength = *readfilelength | ( ( buffer[++i]+128)<<(j*8));
	//printf("Buffer[%d]==%d\n",i,buffer[i]);
	//printf("readfilelength = %lu\n", *readfilelength);
    }

    //printf("file length is %lu\n", *readfilelength);



}
