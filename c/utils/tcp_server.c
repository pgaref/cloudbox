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

#define TCP_PORT 4444 
#define BACKLOG 5
#define LENGTH 512 

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int main ()
{
	/* Defining Variables */
	int sockfd, nsockfd, sin_size; 
	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
	char revbuf[LENGTH]; // Receiver buffer
	int start = 0,i=0;
	char fname[LENGTH];
	
	/* Get the Socket file descriptor */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}
	else 
		printf("[TCP Server] Obtaining socket descriptor successfully.\n");
	
	/* Fill the client socket address struct */
	addr_local.sin_family = AF_INET; // Protocol Family
	addr_local.sin_port = htons(TCP_PORT); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct
	
	/* Bind a special Port */
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		exit(1);
	}
	else 
		printf("[TCP Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n",TCP_PORT);
	
	/* Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1)
	{
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
		printf ("[TCP Server] Listening the port %d successfully.\n", TCP_PORT);
	
	int success = 0;
	while(success == 0)
	{
		sin_size = sizeof(struct sockaddr_in);
		
		/* Wait a connection, and obtain a new socket file despriptor for single connection */
		if ((nsockfd = accept(sockfd, (struct sockaddr *)&addr_remote,(socklen_t *) &sin_size)) == -1) 
		{
			fprintf(stderr, "ERROR: Obtaining new Socket Despcritor. (errno = %d)\n", errno);
			exit(1);
		}
		else{ 
			printf("[Server] Server got connection from %s.\n", inet_ntoa(addr_remote.sin_addr));
		}
		/*Receive File from Client */
		char* fr_name = "./receive.txt";
		FILE *fr = fopen(fr_name, "ab");
		if(fr == NULL)
			printf("File %s Cannot be opened file on server.\n", fr_name);
		else
		{
			bzero(revbuf, LENGTH); 
			int fr_block_sz = 0;
			while((fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0)) > 0) 
			{
				if(!start){
					/* This 'if' loop will executed almost once i.e. until 
					 *				 getting the *file name */
					for (i = 0; i < LENGTH; i++)
					{
						/* Since '#' is the termination character for file name */
						if (revbuf[i] == '#')
						{
							start = 1;       // Got the file name
							break;
						}
						
						fname[i] = revbuf[i]; //Storing the file name in fname
					}
					
					fname[i] = '\0';
					printf("GOT NAME : %s, len %zd \n",fname, strlen(fname));
					
				}
				else{
					int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
					if(write_sz < fr_block_sz)
					{
						error("File write failed on server.\n");
					}
					bzero(revbuf, LENGTH);
					if (fr_block_sz == 0 || fr_block_sz != 512) 
						break;
				}
			}
			if(fr_block_sz < 0)
			{
				if (errno == EAGAIN)
				{
					printf("recv() timed out.\n");
				}
				else
				{
					fprintf(stderr, "recv() failed due to errno = %d\n", errno);
					exit(1);
				}
			}
			printf("Ok received from client!\n");
			start = 0 ; /* Start over, waiting for new file!*/
			fclose(fr); 
		}
		
		/* Call the Script 
		 *	system("cd ; chmod +x script.sh ; ./script.sh");
		 * 
		 *	/* Send File to Client 
		 *	//if(!fork())
		 *	//{
		 *	    char* fs_name = "/home/aryan/Desktop/output.txt";
		 *	    char sdbuf[LENGTH]; // Send buffer
		 *	    printf("[Server] Sending %s to the Client...", fs_name);
		 *	    FILE *fs = fopen(fs_name, "r");
		 *	    if(fs == NULL)
		 *	    {
		 *	        fprintf(stderr, "ERROR: File %s not found on server. (errno = %d)\n", fs_name, errno);
		 *			exit(1);
		 }
		 
		 bzero(sdbuf, LENGTH); 
		 int fs_block_sz; 
		 while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0)
		 {
			 if(send(nsockfd, sdbuf, fs_block_sz, 0) < 0)
			 {
				 fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
				 exit(1);
		 }
		 bzero(sdbuf, LENGTH);
		 }
		 printf("Ok sent to client!\n");
		 success = 1;
		 close(nsockfd);
		 printf("[Server] Connection with Client closed. Server will wait now...\n");
		 while(waitpid(-1, NULL, WNOHANG) > 0);
		 //}*/
		 }
		 }