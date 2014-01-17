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
 * build instructions
 *
 * gcc -o bclient bclient.c
 *
 * Usage:
 * ./bclient <serverport>
 */

/* Maximum size of a datagram in most of the cases! */
#define MAXBUF 65535

int main(int argc, char*argv[])
{
	int sock, status, buflen, sinlen;
	char buffer[MAXBUF];
	struct sockaddr_in sock_in;
	int yes = 1;
	
	sinlen = sizeof(struct sockaddr_in);
	memset(&sock_in, 0, sinlen);
	buflen = MAXBUF;
	
	sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock==-1)
		perror("Cloudbox Error: UDP Broadcast Client socket not created\n");
	
	/*Fill in client's sockaddr_in */
	sock_in.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_in.sin_port = htons(0);
	sock_in.sin_family = PF_INET;
	
	status = bind(sock, (struct sockaddr *)&sock_in, sinlen);
	if(status == -1)
		perror("Cloudbox Error: UDP Broadcast Client bind failed\n");
	
	status = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(int) );
	if(status == -1)
		perror("Cloudbox Error: UDP Broadcast Client setsockopt call failed\n");
	
	/* -1 = 255.255.255.255 this is a BROADCAST address,
	 *    a local broadcast address could also be used.
	 *    you can comput the local broadcat using NIC address and its NETMASK 
	 */ 
	
	sock_in.sin_addr.s_addr=htonl(-1); /* send message to 255.255.255.255 */
	sock_in.sin_port = htons(atoi(argv[1])); /* port number */
	sock_in.sin_family = PF_INET;
	
	sprintf(buffer, argv[2]);
	buflen = strlen(buffer);
	status = sendto(sock, buffer, buflen, 0, (struct sockaddr *)&sock_in, sinlen);
	if (status ==-1)
		perror("Cloudbox Error: UDP Broadcast Client sendto call failed");
	
	shutdown(sock, 2);
	close(sock);
}
/*
int main( int argc, char *argv[])
{
	char buf[512];
	int clientToServer_socket;
	unsigned int fromLength;
	struct sockaddr_in Remote_Address, Server_Address;
	struct hostent *hostPointer;
	int message, checkCall;
	int counter1 = 0;
	int counter2 = 0;
	int broadcastOn = 1;
	int broadcastOff = 0;
	char *broadcastMessage;
	
	broadcastMessage = "Hello";
	
	printf("Message %s\n", broadcastMessage);
	
	/*Create client socket
	clientToServer_socket=socket(AF_INET, SOCK_DGRAM, 0);
	if (clientToServer_socket==-1)
		perror("Error: client socket not created");
	
	/*Fill in client's sockaddr_in 
	bzero(&Remote_Address, sizeof(Remote_Address));
	Remote_Address.sin_family=AF_INET;
	hostPointer=gethostbyname(argv[1]);
	memcpy((unsigned char * ) &Remote_Address.sin_addr, (unsigned char *) hostPointer -> h_addr, hostPointer -> h_length);
	Remote_Address.sin_port=htons(atoi(argv[2]));
	
	checkCall = setsockopt(clientToServer_socket, SOL_SOCKET, SO_BROADCAST, &broadcastOn, 4);
	if(checkCall == -1)
		perror("Error: setsockopt call failed");
	
	Remote_Address.sin_addr.s_addr|=htonl(0x1ff);
	
	checkCall = setsockopt(clientToServer_socket, SOL_SOCKET, SO_BROADCAST, &broadcastOff, 4);
	if(checkCall == -1)
		perror("Error: Second setsockopt call failed");
	
	int broadcastMessageLen = strlen(broadcastMessage) + 1;
	
	printf("Message test %d\n", broadcastMessageLen);
	
	
	message = sendto(clientToServer_socket, broadcastMessage, broadcastMessageLen, 0, (struct sockaddr *) &Remote_Address, sizeof(Remote_Address));
	if (message ==-1)
		perror("Error: sendto call failed");
	
	while(1)
	{
		fromLength = sizeof(Server_Address);
		message = recvfrom(clientToServer_socket, buf, 512, 0, (struct sockaddr *) &Server_Address, &fromLength);
		if (message ==-1)
			perror("Error: recvfrom call failed");
		
		printf("SERVER: read %d bytes from IP %s(%s)\n", message, inet_ntoa(Server_Address.sin_addr), buf);
	}
	close(clientToServer_socket);
	
}*/