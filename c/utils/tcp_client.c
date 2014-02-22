#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <netdb.h>

#define LENGTH 512 


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	/* Variable Definition */
	int sockfd;
	char *tmp;
	struct sockaddr_in remote_addr;

    if(argc != 4){
        printf("Usage ./client <IP> <Port> <Filename>\n");
        exit(-1);
    }

	/* Get the Socket file descriptor */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor! (errno = %d)\n",errno);
		exit(1);
	}

	/* Fill the socket address struct */
	remote_addr.sin_family = AF_INET; 
	remote_addr.sin_port = htons(atoi(argv[2])); 
	inet_pton(AF_INET, argv[1], &remote_addr.sin_addr); 
	bzero(&(remote_addr.sin_zero), 8);

	/* Try to connect the remote */
	if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to connect to the host! (errno = %d)\n",errno);
		exit(1);
	}
	else 
		printf("[TCP Client] Connected to server at port %d...ok!\n", atoi(argv[2]));

	/* Send File to Server */
    char* fs_name = argv[3];
    char sdbuf[LENGTH]; 
    printf("[TCP Client] Sending %s to the Server... ", fs_name);
    FILE *fs = fopen(fs_name, "rb");
    if(fs == NULL)
    {
        printf("ERROR: File %s not found.\n", fs_name);
        exit(1);
    }
    
    tmp = (char *)malloc(sizeof (char) * (strlen(fs_name) + 2));
    strcpy(tmp, fs_name);          //Copying the file name with tmp 
    strcat(tmp, "#");            //Appending '#' to tmp
    
    memset(sdbuf, '\0', LENGTH);
    strcpy(sdbuf, tmp);         //Now copying the tmp value to buffer
    if(send(sockfd, sdbuf, LENGTH, 0) < 0)
    {
        fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
        exit(-1);
    }
    
    bzero(sdbuf, LENGTH);
    int fs_block_sz; 
    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
    {
        if(send(sockfd, sdbuf, fs_block_sz, 0) < 0)
        {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
            break;
        }
        bzero(sdbuf, LENGTH);
    }
    printf("Ok File %s from Client was Sent!\n", fs_name);

	close (sockfd);
	printf("[TCP Client] Closing connection!\n");
	return (0);
}