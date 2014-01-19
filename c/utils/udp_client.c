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

#define SHA1_BYTES_LEN 20
/**
 * Maximum size of a datagram in most of the cases! 
 */
#define MAXBUF 65535

/**
 * An enum type indicating the type of each message
 * send by the client.
 * 
 * Message type should consist the TWO FIRST BYTES
 * of each packet if necessary. It helps the receiver
 * to figure out what to do.
 * 
 * NOTE: By default enum types are integers so they
 * are four bytes. Take care of it and cast
 * the message type to a proper 2-byte type.
 */
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
 * Encode UDP packet according to the given instructions!
 */
 char packet[MAXBUF];
int udp_packet_encode(msg_type_t type, char * client_name, int tcp_port){//, time_t, curr_time, time_t mod_time, char * filename, char *sha){
	printf("wtf ?? %s \n",client_name);
	int packet_count =0, i=0;
	short b = (short) type;
	memcpy(&packet, &b, 2);
	packet[2] = 0;
	packet_count =3;
	for(i = 0; i < strlen(client_name); i++){
		packet[packet_count++] = client_name[i];
		printf("i %d count %d pak %c \n", i, packet_count ,packet[packet_count-1]);
	}
	packet[packet_count] = 0;
	
	
	printf("TYPE : %d \n", type);
	printf("Pak : %d %d \n", packet[0], packet[1]);
	printf("Size: %d\n", packet_count);
	
	return packet_count;

}

int main(int argc, char*argv[])
{
	int sock, status, buflen, sinlen;
	char * buffer;
	
	struct sockaddr_in sock_in;
	int yes = 1;
	
	sinlen = sizeof(struct sockaddr_in);
	memset(&sock_in, 0, sinlen);
	
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
	
	char * test_send = "BickDick";
	buflen = udp_packet_encode(STATUS_MSG,test_send,44);
	printf("ERa!  %d \n",buflen);
	status = sendto(sock, packet, buflen, 0, (struct sockaddr *)&sock_in, sinlen);
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