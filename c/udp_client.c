#include "cloudbox.h"
extern int broadcast_port;
extern char *client_name;
extern int TCP_PORT;

/**
 * Decode a message and return the client name
 */
char * udp_packet_clientName(char * packet){
	char * c_name = (char *) malloc(255);
	int count =3 , i=0;
	while(packet[count] != 0){
		c_name[i++] = packet[count++];
	}c_name[i] = '\0';
	return c_name;
}

/*
 * Encode UDP packet_to_send according to the given instructions!
 */
 char packet_to_send[MAXBUF];
int udp_packet_encode(msg_type_t type, char * client_name, int tcp_port, time_t mod_time){//, time_t, curr_time, time_t mod_time, char * filename, char *sha){
	
	int packet_count =0, i=0;
	uint16_t b = (uint16_t) type;
	time_t clk = time(NULL);
	memcpy(&packet_to_send, &b, 2);
	packet_to_send[2] = 0;
	packet_count =3;
	for(i = 0; i < strlen(client_name); i++){
		packet_to_send[packet_count++] = client_name[i];
	}
	packet_to_send[packet_count++] = 0;
	b = (uint16_t)tcp_port;
	
	memcpy(&packet_to_send[packet_count], &b , 2);
	packet_count+=2;
	
	memcpy(&packet_to_send[packet_count], &clk, 8);
	packet_count+=8;
	memcpy(&packet_to_send[packet_count], &mod_time,8);
	packet_count+=8;
	
	/*
	printf("TYPE : %d \n", type);
	printf("Pak : %d %d \n", packet_to_send[0], packet_to_send[1]);
	printf("Tcp Port: %u \n", b);
	printf("Size: %d\n", packet_count);
	printf("Current Time %s \n",ctime(&clk));
	printf("File modification Time %s \n",ctime(&mod_time));
	*/
	return packet_count;

}

void udp_packet_send(){
	int sock, status, buflen, sinlen;
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
	sock_in.sin_port = htons(broadcast_port); /* port number */
	sock_in.sin_family = PF_INET;
	
	buflen = udp_packet_encode(STATUS_MSG,client_name,TCP_PORT,time(NULL));
	status = sendto(sock, packet_to_send, buflen, 0, (struct sockaddr *)&sock_in, sinlen);
	if (status ==-1)
		perror("Cloudbox Error: UDP Broadcast Client sendto call failed");
	
	shutdown(sock, 2);
	close(sock);
}