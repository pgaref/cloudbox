#include "cloudbox.h"
#define LENGTH 512 


/*void error(const char *msg)
{
	perror(msg);
	exit(1);
}*/
/* Send file to the destination */
int send_file(char * client_ip, uint16_t port, char * filename){
	/* Variable Definition */
	int sockfd;
	char *tmp;
	struct sockaddr_in remote_addr;
	
	/* Get the Socket file descriptor */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor! (errno = %d)\n",errno);
		exit(1);
	}
	
	/* Fill the socket address struct */
	remote_addr.sin_family = AF_INET; 
	remote_addr.sin_port = htons(port); 
	inet_pton(AF_INET, client_ip, &remote_addr.sin_addr); 
	bzero(&(remote_addr.sin_zero), 8);
	
	/* Try to connect the remote */
	if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to connect to the host! (errno = %d)\n",errno);
		exit(1);
	}
	else{
		printf("\t[TCP Client] is going to transfer %s \n", filename);	
		printf("\t[TCP Client] Connected to server at port %d...ok!\n", port);
	}
	/*Concat path + filename to get the correct stats */
	char * fullpath = malloc(strlen(watched_dir) + strlen(filename) + 1);
	strcpy(fullpath, watched_dir);
	strcat(fullpath, filename);
	char* fs_name = fullpath;
	/* Send File to Server */
	char sdbuf[LENGTH]; 
	printf("\t[TCP Client] Sending %s to the Server... ", fs_name);
	FILE *fs = fopen(fs_name, "r");
	if(fs == NULL)
	{
		printf("ERROR: File %s not found.\n", fs_name);
		exit(1);
	}
	
	tmp = (char *)malloc(sizeof (char) * (strlen(filename) + 2));
	strcpy(tmp, filename);          //Copying the file name with tmp 
	strcat(tmp, "#");            //Appending '#' to tmp
	
	memset(sdbuf, '\0', LENGTH);
	strcpy(sdbuf, tmp);         //Now copying the tmp value to buffer
	if(send(sockfd, sdbuf, LENGTH, 0) < 0)
	{
		fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", filename, errno);
		exit(-1);
	}
	
	bzero(sdbuf, LENGTH);
	int fs_block_sz; 
	while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
	{
		if(send(sockfd, sdbuf, fs_block_sz, 0) < 0)
		{
			fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", filename, errno);
			break;
		}
		bzero(sdbuf, LENGTH);
	}
	printf("\t[TCP Client] Ok File %s from Client was Sent!\n", filename);
	
	close (sockfd);
	printf("\t[TCP Client] Closing connection!\n");
	return (0);
}