#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>


/*
 * build instructions:
 *
 * gcc -o bserver bserver.c
 *
 * Usage:
 * ./bserver
 */
#define SHA1_BYTES_LEN 20
#define MAXBUF 65535
typedef enum {
	STATUS_MSG = 0x1, 				/**< A status message */
	NO_CHANGES_MSG = 0x2, 			/**< Client's directory is clean */
	NEW_FILE_MSG = 0x3, 			/**< A new file added at the client */
	FILE_CHANGED_MSG = 0x4, 		/**< A file changed at the client */
	FILE_DELETED_MSG = 0x5, 		/**< A file deleted at the client */
	FILE_TRANSFER_REQUEST = 0x6,	/**< Another client wants our file */
	FILE_TRANSFER_OFFER = 0x7,		/**< Another client sends us a file */
	DIR_EMPTY = 0x08,				/**< The directory of the client has no files */
	NOP = 0xff						/**< Do nothing. Use it for debug reasons */
} msg_type_t;


/**
 * A linked list (double or single, choose what you want)
 * that keeps appropriate info for each file at the 
 * directory that the client scans for changes.
 * 
 * NOTE: Feel free to add your own fields
 */
typedef struct dir_files_status_list {
	char *filename;
	off_t size_in_bytes;
	char sha1sum[SHA1_BYTES_LEN];
	time_t modifictation_time_from_epoch;
	/* Extra fields! */
	char  * owner;
	char  * group;
	mode_t  permission;

	struct dir_files_status_list *next;
} dir_files_status_list;


/*
 * Decode UDP packet according to the given instructions!
 */
void udp_packet_decode(char * packet){
	char pak[MAXBUF];
	time_t clk, mod_time;
	char client_name[255];
	short tcp_port;
	int count =3 , i=0;
	char tmp[2];
	
	memcpy(pak, packet,MAXBUF);
	tmp[0] = pak[0];
	tmp[1] = pak[1];
	
	switch(tmp[0]){
		case(1):
			printf("\tSTATUS_MSG \n");
			break;
		case(2):
			printf("\tNO_CHANGES_MSG \n");
			break;
		case(3):
			printf("\tNEW_FILE_MSG \n");
			break;
		case(4):
			printf("\tFILE_CHANGED_MSG \n");
			break;
		case(5):
			printf("\tFILE_DELETED_MSG \n");
			break;
		case(6):
			printf("\tFILE_TRANSFER_REQUEST \n");
			break;
		case(7):
			printf("\tFILE_TRANSFER_OFFER \n");
			break;
		case(8):
			printf("\tDIR_EMPTY \n");
			break;
		case(-1):
			printf("\tNOP \n");
			break;
		default:
			printf("Not a Valid Status: %d %d \n", tmp[0],tmp[1]);
			break;
	}
	
	if(pak[2] != 0)
		perror("Not a Valid Message Field Client_name\n");
	while(pak[count] != 0){
		client_name[i++] = pak[count++];
	}client_name[i] = '\0';
	count++;
	
	memcpy(&tcp_port, &pak[count], 2);
	count+=2;
	
	memcpy(&clk, &pak[count],8);
	count+=8;
	/*
	memcpy(&mod_time, &pak[count],8);
	count+=8;
	*/
	
	
	
	
	
	printf("\tClient Name: %s len:%d i:%d count:%d\n", client_name, strlen(client_name), i,count);
	printf("\tTCP Listening Port: %d \n", tcp_port);
	printf("\tPacket Sent at: %s \n", ctime(&clk));
	/*
	printf("File modification Time %s \n",ctime(&mod_time));
	*/

}


int main()
{
	int sock, status, buflen;
	unsigned sinlen;
	char buffer[MAXBUF];
	struct sockaddr_in sock_in;
	int yes = 1;
	
	sinlen = sizeof(struct sockaddr_in);
	memset(&sock_in, 0, sinlen);
	/* Create Socket */
	sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == -1)
		perror("Cloudbox Error: UDP Broadcast Server Socket failed!\n");
	
	/*Fill in server's sockaddr_in */
	sock_in.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_in.sin_port = htons(5555);
	sock_in.sin_family = PF_INET;
	
	/*Bind server socket and listen for incoming client */
	status = bind(sock, (struct sockaddr *)&sock_in, sinlen);
	if(status == -1)
		perror("Cloudbox Error: UDP Broadcast Server Bind-ing failed\n");
	
	status = getsockname(sock, (struct sockaddr *)&sock_in, &sinlen);
	printf("Sock port %d\n",htons(sock_in.sin_port));
	
	buflen = MAXBUF;
	printf("UDP SERVER: waiting for data from client\n");
	while(1)
	{
		memset(buffer, 0, buflen);
		status = recvfrom(sock, buffer, buflen, 0, (struct sockaddr *)&sock_in, &sinlen);
		if(status == -1)
			perror("Cloudbox Error: UDP Broadcast Server recvfrom call failed \n");
		//Decode!
		udp_packet_decode(buffer);
		
		if(!strcmp(buffer, "quit")){
			printf("Quiting. . .\n");
			break;
		}
		printf("UDP SERVER: read %d bytes from IP %s:%d(%s)\n", status,
		       inet_ntoa(sock_in.sin_addr),sock_in.sin_port, buffer);
		
	}
	shutdown(sock, 2);
	close(sock);
}


/*
int main( int argc, char *argv[])
{
	int server_socket;
	struct sockaddr_in server_address, client_address;
	char buf[512];
	unsigned int clientLength;
	int checkCall, message;
	
	/*Create socket 
	server_socket=socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket == -1)
		perror("Error: socket failed");
	
	bzero((char*) &server_address, sizeof(server_address));
	
	/*Fill in server's sockaddr_in
	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);
	server_address.sin_port=htons(atoi(argv[1]));
	
	/*Bind server socket and listen for incoming clients
	checkCall = bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr));
	if(checkCall == -1)
		perror("Error: bind call failed");
	
	while(1)
	{
		printf("SERVER: waiting for data from client\n");
		
		clientLength = sizeof(client_address);
		message = recvfrom(server_socket, buf, sizeof(buf), 0,
				   (struct sockaddr*) &client_address, &clientLength);
		if(message == -1)
			perror("Error: recvfrom call failed");
		
		printf("SERVER: read %d bytes from IP %s:%d(%s)\n", message,
		       inet_ntoa(client_address.sin_addr),client_address.sin_port, buf);
		
		if(!strcmp(buf,"quit"))
			break;
		
		strcpy(buf,"ok");
		
		message = sendto(server_socket, buf, strlen(buf)+1, 0,
				 (struct sockaddr*) &client_address, sizeof(client_address));
		if(message == -1)
			perror("Error: sendto call failed");       
		
		printf("SERVER: send completed\n");
	}
	checkCall = close(server_socket);
	if(checkCall == -1)
		perror("Error: bind call failed");
	
}*/