#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>


/*
 * build instructions:
 *
 * gcc -o bserver bserver.c
 *
 * Usage:
 * ./bserver
 */

#define MAXBUF 65535

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
	printf("SERVER: waiting for data from client\n");
	while(1)
	{
		memset(buffer, 0, buflen);
		status = recvfrom(sock, buffer, buflen, 0, (struct sockaddr *)&sock_in, &sinlen);
		if(status == -1)
			perror("Cloudbox Error: UDP Broadcast Server recvfrom call failed \n");
		
		if(!strcmp(buffer, "quit")){
			printf("Quiting. . .\n");
			break;
		}
		printf("SERVER: read %d bytes from IP %s:%d(%s)\n", status,
		       inet_ntoa(sock_in.sin_addr),sock_in.sin_port, buffer);
		
		
		//printf("sendto Status = %d\n", status);
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