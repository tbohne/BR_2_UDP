#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main (int argc, char *argv[]) {
	
	int sockfd, length, err;
	struct sockaddr_in to, from;
	char buff[256];

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
	bzero(buff, 256);

	// Assign Protocol Family
	to.sin_family = AF_INET;

	// Assign Port
	to.sin_port = htons(atoi(argv[2]));

	// Length of the Address Structure
	length = sizeof(struct sockaddr_in);

	// Address of the Receiver (Dotted to Network)
	to.sin_addr.s_addr = inet_addr(argv[1]);

	// ************************************** NEW *********************************************
	

	struct File {
		char* fileName;
		long fileSize;
		



	};

	// Dateiname ohne Pfad 
	char* fileInfo = argv[3];
	char* sub = strdup(fileInfo);
	fileName = basename(sub);

	long SizeOfFile;
	
	FILE *fp;
	// char str[] = "FOOBAR";
	fp = fopen(fileInfo, "rb");
	// Dateigroesse ermitteln
	fseek(fp, 0L, SEEK_END);
	SizeOfFile = ftell(fp);
	rewind(fp);


	






	/*err = fwrite(str, 1, sizeof(str), fp);
	if (err < 0) {
		printf("fwrite-Problem");
		exit(1);
	}
	*/

	//strcat(fp, buff);
	//printf("%s\n", buff);

	//strcat(new_file, buff);
	//FILE *fp;
	//fp = fopen(new_file, "w+");
	//err = fwrite(buff, 1, sizeof(buff), fp);
	//if (err < 0) {
	//	printf("shit.");
	//	exit(1);
	//}

	printf("%s\n", str);

	// ****************************************************************************************

	// printf("enter something:");
	// Reads a line from the specified stream and stores it into the string pointed to by buff.
	//fgets(buff, 256, stdin);

	err = sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr *)&to, length);

	if (err < 0) {
		printf("sendto-Problem");
		exit(1);
	}

	err = recvfrom(sockfd, buff, 256, 0, (struct sockaddr *)&from, &length);

	if (err < 0) {
		printf("recvfrom-Problem");
		exit(1);
	}

	printf("Got an ACK! %s Port: %d: %s", inet_ntoa(from.sin_addr), htons(from.sin_port), buff);

	// Close Socket
	close(sockfd);

	return 0;
}