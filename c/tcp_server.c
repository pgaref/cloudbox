#include "cloudbox.h"


#define BACKLOG 5
#define LENGTH 512 


extern char * watched_dir;
int TCP_PORT;



void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void * handle_incoming_tcp_connection_thread(void *params)
{
	/* Defining Variables */
	int sockfd, nsockfd, sin_size; 
	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
	char revbuf[LENGTH]; // Receiver buffer
	int start = 0,i=0;
	unsigned sinlen;
	char fname[LENGTH];
	char fr_name[LENGTH];
	
	/* Get the Socket file descriptor */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}
	else 
		printf("\n\t[TCP Server] Obtaining socket descriptor successfully.\n");
	
	/* Fill the client socket address struct */
	addr_local.sin_family = AF_INET; // Protocol Family
	//addr_local.sin_port = htons(TCP_PORT); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct
	
	/* Bind a special Port */
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		exit(1);
	}
	else{
	    sinlen = sizeof(struct sockaddr);
	    getsockname(sockfd, (struct sockaddr *)&addr_local, &sinlen);
	    TCP_PORT = htons(addr_local.sin_port);
		printf("\t[TCP Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n",TCP_PORT);
	}
	/* Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1)
	{
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
		printf ("\t[TCP Server] Listening the port %d successfully.\n", TCP_PORT);
	
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
			printf("\n\t[TCP Server] Server got connection from %s.\n", inet_ntoa(addr_remote.sin_addr));
		}
		/*Receive File from Client */
		FILE *fr = NULL;
    
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
                //printf("GOT NAME : %s, len %zd \n",fname, strlen(fname));
                strcpy(fr_name,watched_dir);
                strcat(fr_name,fname);
                fr = fopen(fr_name, "a");
                if(fr == NULL){
                    fprintf(stderr,"File %s Cannot be opened file on server.\n", fr_name);
                    exit(-1);
                }
                
            }
            else{
                if(fr == NULL){
                    printf("File %s Was not opened!!\n", fr_name);
                    exit(-1);
                }
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
        printf("TCP Transfer => Ok received from client!\n");
        start = 0 ; /* Start over, waiting for new file!*/
        fclose(fr); 
	}
	return 0;
}