#include "cloudbox.h"

/*
 * The list that holds all the current watched files.
 * 
 * It is very convenient this list to be shorted by the file name
 * in order to be able to find immediately inconsistencies,
 */
struct dir_files_status_list * watched_files;
char * watched_dir;

/*
 * Used to remove compiler warnings! 
 */
extern int alphasort();
extern char *strdup(const char *s);
extern int TCP_PORT;

/* Much more convenient to make the global
 * And share them between files using extern!
 */
int broadcast_port;
char *client_name;


/*
 * Print mutex, for printing nicely the messages from different threads
 */
pthread_mutex_t print_mutex;


/* 
 * Mutex used to protect the accesses from different threads
 * of the file list of the watched directory
 */
pthread_mutex_t file_list_mutex;

/*
 * Phase 2 : Stats struct and mutex for locking
 */
struct cloudbox_stats appStats;
pthread_mutex_t stats_mutex;

/*
 * Decode UDP packet according to the given instructions!
 */
void udp_packet_decode(char * packet, char * fromIP){
	struct dir_files_status_list *currTmp, *watchedTmp, *result;
	char pak[MAXBUF];
	char fileSHA[SHA1_BYTES_LEN];
	off_t file_len;
	int64_t clk, mod_time;
	char FromClient[255], file_name[255];
	uint16_t tcp_port;
	int count =3 , i=0;
	char tmp[2], *file_full_path;
	
	
	/* First Decode each field and then print */
	memcpy(pak, packet,MAXBUF);
	tmp[0] = pak[0];
	tmp[1] = pak[1];
	
	if(pak[2] != 0)
		perror("Not a Valid Message Field Client_name\n");
	while(pak[count] != 0){
		FromClient[i++] = pak[count++];
	}FromClient[i] = '\0';
	count++;
	
	memcpy(&tcp_port, &pak[count], 2);
	count+=2;
	
	memcpy(&clk, &pak[count],8);
	count+=8;
	
	if( (tmp[0] >= 3) && (tmp[0] <= 7)){
		memcpy(&mod_time, &pak[count], 8);
		count += 9;
		i=0;
		while(pak[count] != 0){
			file_name[i++] = pak[count++];
		}file_name[i] = '\0';
		count++;
		memcpy(fileSHA, &pak[count], SHA1_BYTES_LEN);
		count+=SHA1_BYTES_LEN;
		memcpy(&file_len, &pak[count], 8);
		count+=8;
		
	}
	/* Get the Lock and start printing!*/
	
	pthread_mutex_lock(&print_mutex);
	switch(tmp[0]){
		case(1):
			printf("\n\tSTATUS_MSG \n");
			break;
		case(2):
			printf("\n\tNO_CHANGES_MSG \n");
			break;
		case(3):
			printf("\n\tNEW_FILE_MSG \n");
			pthread_mutex_lock(&file_list_mutex);
			watchedTmp = watched_files;
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			currTmp->filename = (char *) malloc(strlen(file_name));
			if (!currTmp->filename) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			strcpy(currTmp->filename, file_name);
			
			SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, watchedTmp, currTmp, ILIST_COMPARATOR, next, result);
			/* Case 1: the client does not have the file */
			if(result == NULL){
			
				/* Add file to the list and wait until its received to compare the SHA */
				currTmp->size_in_bytes = file_len;
				currTmp->modifictation_time_from_epoch = mod_time;
				/* deep copy */
				for(i = 0; i < SHA1_BYTES_LEN; i++)
					currTmp->sha1sum[i] = fileSHA[i];
				
				
				SGLIB_SORTED_LIST_ADD(struct dir_files_status_list, watched_files, currTmp, ILIST_COMPARATOR, next);
				
				
				/* Ask for Transfer! */ 
				i = udp_file_packet_encode(FILE_TRANSFER_REQUEST,client_name,TCP_PORT,&clk,&mod_time, file_name,fileSHA,file_len);
				currTmp->processed = TRUE;
				udp_packet_send(i);
				currTmp->processed = FALSE;
				
			}
			/* Case 2: the client DOES have the file listed */
			else{
				
				if(result->modifictation_time_from_epoch < mod_time){
					/* Ask for Transfer! */ 
					i = udp_file_packet_encode(FILE_TRANSFER_REQUEST,client_name,TCP_PORT,&clk,&mod_time, file_name,fileSHA,file_len);
					currTmp->processed = TRUE;
					udp_packet_send(i);
					currTmp->processed = FALSE;
				}
				free(currTmp);
			}
			pthread_mutex_unlock(&file_list_mutex);
			break;
		case(4):
			printf("\n\tFILE_CHANGED_MSG \n");
			pthread_mutex_lock(&file_list_mutex);
			watchedTmp = watched_files;
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			currTmp->filename = (char *) malloc(strlen(file_name));
			if (!currTmp->filename) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			strcpy(currTmp->filename, file_name);
			SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, watchedTmp, currTmp, ILIST_COMPARATOR, next, result);
			/* Case 1: the client does not have the file, so add and transfer */
			if(result == NULL){
			
				/* Add file to the list and wait until its received to compare the SHA */
				currTmp->size_in_bytes = file_len;
				currTmp->modifictation_time_from_epoch = mod_time;
				/* deep copy */
				for(i = 0; i < SHA1_BYTES_LEN; i++)
					currTmp->sha1sum[i] = fileSHA[i];
				
				
				SGLIB_SORTED_LIST_ADD(struct dir_files_status_list, watched_files, currTmp, ILIST_COMPARATOR, next);
				
				
				/* Ask for Transfer! */ 
				i = udp_file_packet_encode(FILE_TRANSFER_REQUEST,client_name,TCP_PORT,&clk,&mod_time, file_name,fileSHA,file_len);
				/* file does not exist so its ok to set processed without checking */
				currTmp->processed = TRUE;
				udp_packet_send(i);
				currTmp->processed = FALSE;
				
			}
			/* Case 2: the client DOES have the file listed so update it!*/
			else{
				/*Check timestamp */
				if(result->modifictation_time_from_epoch < mod_time){
					
					/* Add file to the list and wait until its received to compare the SHA */
					result->size_in_bytes = file_len;
					result->modifictation_time_from_epoch = mod_time;
					/* deep copy */
					for(i = 0; i < SHA1_BYTES_LEN; i++)
						result->sha1sum[i] = fileSHA[i];
					
					/* Ask for Transfer! */ 
					i = udp_file_packet_encode(FILE_TRANSFER_REQUEST,client_name,TCP_PORT,&clk,&mod_time, file_name,fileSHA,file_len);
					if (result->processed == FALSE){
						result->processed = TRUE;
						udp_packet_send(i);
						result->processed = FALSE;
					}
				}
				free(currTmp);
			}
			pthread_mutex_unlock(&file_list_mutex);
			break;
		case(5):
			printf("\n\tFILE_DELETED_MSG \n");
			pthread_mutex_lock(&file_list_mutex);
			watchedTmp = watched_files;
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			currTmp->filename = (char *) malloc(strlen(file_name));
			if (!currTmp->filename) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			strcpy(currTmp->filename, file_name);
			SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, watchedTmp, currTmp, ILIST_COMPARATOR, next, result);
			/* Case 1: the client does have the file */
			if(result != NULL){
			
				/* Check file similarity! */
				if((file_len == result->size_in_bytes) && (compare_sha1(result->sha1sum,fileSHA) == 0))
				{
					/* remove from dir */
					file_full_path = (char * )malloc(strlen(file_name) + strlen(watched_dir)+1);
					strcpy(file_full_path,watched_dir);
					strcat(file_full_path,file_name);
					
					/*now remove from list*/
					
					if (result->processed == FALSE){
						result->processed = TRUE;
						if(remove(file_full_path) != 0 ){
							fprintf(stderr,"[Cloudbox] Error deleting file %s\n",file_full_path);
							exit(EXIT_FAILURE);
						}
						SGLIB_LIST_DELETE(struct dir_files_status_list, watchedTmp, result, next);
						result->processed = FALSE;
					}
					
					
					free(file_full_path);
					
				}
				
			}
			
			/* Case 2: the client DOES NOT have the file Deleted */
			else{
				free(currTmp);
			}
			pthread_mutex_unlock(&file_list_mutex);
			break;
		case(6):
			printf("\n\tFILE_TRANSFER_REQUEST \n");
			pthread_mutex_lock(&file_list_mutex);
			watchedTmp = watched_files;
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			currTmp->filename = (char *) malloc(strlen(file_name));
			if (!currTmp->filename) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			strcpy(currTmp->filename, file_name);
			SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, watchedTmp, currTmp, ILIST_COMPARATOR, next, result);
			if((result != NULL) && (compare_sha1(result->sha1sum,fileSHA) == 0) ){
				/* its my file the other client is looking for! 
				 */
				if (result->processed == FALSE){
					result->processed = TRUE;
					send_file(fromIP, tcp_port, file_name);
					result->processed = FALSE;
				}
			}
			pthread_mutex_unlock(&file_list_mutex);
			free(currTmp->filename);
			free(currTmp);
			break;
		case(7):
			printf("\n\tFILE_TRANSFER_OFFER \n");
			pthread_mutex_lock(&file_list_mutex);
			watchedTmp = watched_files;
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			currTmp->filename = (char *) malloc(strlen(file_name));
			if (!currTmp->filename) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			strcpy(currTmp->filename, file_name);
			
			SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, watchedTmp, currTmp, ILIST_COMPARATOR, next, result);
			/* Case 1: the client does not have the file */
			if(result == NULL){
			
				/* Add file to the list and wait until its received to compare the SHA */
				currTmp->size_in_bytes = file_len;
				currTmp->modifictation_time_from_epoch = mod_time;
				/* deep copy */
				for(i = 0; i < SHA1_BYTES_LEN; i++)
					currTmp->sha1sum[i] = fileSHA[i];
				
				
				SGLIB_SORTED_LIST_ADD(struct dir_files_status_list, watched_files, currTmp, ILIST_COMPARATOR, next);
				
				/* Ask for Transfer! */ 
				i = udp_file_packet_encode(FILE_TRANSFER_REQUEST,client_name,TCP_PORT,&clk,&mod_time, file_name,fileSHA,file_len);
				currTmp->processed = TRUE;
				udp_packet_send(i);
				currTmp->processed = FALSE;
				
			}
			/* Case 2: the client DOES have the file listed */
			else{
				free(currTmp);
			}
			pthread_mutex_unlock(&file_list_mutex);
			break;
		case(8):
			printf("\n\tDIR_EMPTY \n");
			pthread_mutex_unlock(&print_mutex);
			/* sleep is useful in case we remove all files from directory and we have both DELETE FILE and EMPTY DIR messages */
			sleep(2);
			watchedTmp = listWatchedDir(watched_dir);
			watched_files = watchedTmp;
			pthread_mutex_lock(&print_mutex);
			pthread_mutex_lock(&file_list_mutex);
			currTmp = (struct dir_files_status_list * ) malloc( sizeof (struct dir_files_status_list));
			if (!currTmp) {
				fprintf(stderr, "malloc() failed: insufficient memory!\n");
				exit(EXIT_FAILURE);
			}
			int listlen=0;
			SGLIB_LIST_LEN(struct dir_files_status_list,watchedTmp,next, listlen);
			UNUSED(currTmp);
			/* If other client is empty start updating him */
			if(listlen > 0){
				SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, watchedTmp, currTmp, next, {
					
					/* Offer file! */ 
					i = udp_file_packet_encode(FILE_TRANSFER_OFFER,client_name,TCP_PORT,&clk,&currTmp->modifictation_time_from_epoch, currTmp->filename,currTmp->sha1sum,currTmp->size_in_bytes);
					if (currTmp->processed == FALSE){
						currTmp->processed = TRUE;
						udp_packet_send(i);
						currTmp->processed = FALSE;
					}
					
				});
			
			
			}
			pthread_mutex_unlock(&file_list_mutex);
			free(currTmp);
			
			
			break;
		case(-1):
			printf("\n\tNOP \n");
			break;
		default:
			printf("\n\tNot a Valid Status: %d %d \n", tmp[0],tmp[1]);
			break;
	}
	
	printf("\tClient Name: %s\n", FromClient);
	printf("\tTCP Listening Port: %u \n", tcp_port);
	printf("\tPacket Sent at: %s", ctime((time_t *)&clk));
	
	/* 
	 * Case of complex message with file fields
	 * time_t mod_time, char * filename, char *sha,off_t file_size
	 */
	if( (tmp[0] >= 3) && (tmp[0] <= 7)){
		printf("\tFile modification Time: %s\n",ctime((time_t *)&mod_time));
		printf("\tFile Name: %s\n", file_name);
		printf("\tFile SHA: ");
		print_sha1(fileSHA);
		printf(" \n");
		printf("\tFile Size: %5jd\n", (intmax_t)file_len);
		
	}
	printf("\n-------- Packet Count = %d-----------\n" ,count);
    pthread_mutex_unlock(&print_mutex);
}
 
 
/**
 * Thread that receives UDP broadcast messages and
 * based on the message type, the dispatcher 
 * can fire up a new thread or do a specific job.
 */

void * udp_receiver_dispatcher_thread(void *port){
	int sock, status, buflen;
	unsigned sinlen;
	char buffer[MAXBUF];
	struct sockaddr_in sock_in;
	sinlen = sizeof(struct sockaddr_in);
	
	
	memset(&sock_in, 0, sinlen);
	/* Create Socket */
	sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == -1)
		perror("Cloudbox Error: UDP Broadcast Server Socket failed!\n");
	
	/*Fill in server's sockaddr_in */
	sock_in.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_in.sin_port = htons((intptr_t)port);
	sock_in.sin_family = PF_INET;
	
	/*Bind server socket and listen for incoming client */
	status = bind(sock, (struct sockaddr *)&sock_in, sinlen);
	if(status == -1)
		perror("Cloudbox Error: UDP Broadcast Server Bind-ing failed\n");
	
	buflen = MAXBUF;
	pthread_mutex_lock(&print_mutex);
	printf("UDP SERVER: waiting for data from client\n");
	pthread_mutex_unlock(&print_mutex);
	while(1)
	{
		memset(buffer, 0, buflen);
		status = recvfrom(sock, buffer, buflen, 0, (struct sockaddr *)&sock_in, &sinlen);
		if(status == -1)
			perror("Cloudbox Error: UDP Broadcast Server recvfrom call failed \n");
		
		/* Decoding if message is not from localhost!! */
        if(strcmp(udp_packet_clientName(buffer),client_name)== 0)
            continue;
		udp_packet_decode(buffer,inet_ntoa(sock_in.sin_addr));
		if(!strcmp(buffer, "quit")){
			printf("Quiting. . .\n");
			break;
		}
		pthread_mutex_lock(&stats_mutex);
		appStats.msg_num += 1;
		appStats.msg_size += status;
		pthread_mutex_unlock(&stats_mutex);
		
		pthread_mutex_lock(&print_mutex);
		printf("UDP SERVER: read %d bytes from IP %s:%d\n", status,
		       inet_ntoa(sock_in.sin_addr),sock_in.sin_port);
		pthread_mutex_unlock(&print_mutex);
	}
	shutdown(sock, 2);
	close(sock);
	return 0;
}
/**
 * Computes the SHA1 checksum of a file.
 */
void compute_sha1_of_file(char *outbuff, char *filename){
	/* outbuff now contains the 20-byte SHA-1 hash */
	FILE *inFile = fopen (filename, "rb");
	if(inFile == NULL){
        fprintf(stderr,"\n Could not open File %s \n", filename);
		exit(-1);
    }
	SHA_CTX shaContext;
	int bytes;
	unsigned char data[1024];
	
	if (inFile == NULL) {
		fprintf (stderr,"%s can't be opened.\n",filename);
		exit(EXIT_FAILURE);
	}
	
	SHA1_Init (&shaContext);
	
	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
	{
		SHA1_Update (&shaContext, data, bytes);
	}
	SHA1_Final ((unsigned char *)outbuff,&shaContext);
	/*
	for(i = 0; i < SHA1_BYTES_LEN; i++)
		printf("%02x", (unsigned char)outbuff[i]);
	
	printf (" %s\n", filename);
	*/
	fclose (inFile);
	
}
/**
 * Computes the SHA1 checksum of a buffer.
 */
void compute_sha1_of_buffer(char *outbuff, char *buffer, size_t buffer_len){
	SHA1((const unsigned char *)buffer, buffer_len, (unsigned char *)outbuff);
	/* outbuff now contains the 20-byte SHA-1 hash */
}

/*  
 *	Compares two SHA hashes and returns 0 if they are the same or 1 if they are not
 */
int compare_sha1(char * sha1, char * sha2){
	int i = 0;
	for(i = 0; i < SHA1_BYTES_LEN; i++){
			if( sha1[i] != sha2[i])
				return 1;
	}
	return 0;
}
/*
 * Prints a readable representation of SHA hash
 */
void print_sha1(char * sha1){
int i;
for(i = 0; i < SHA1_BYTES_LEN; i++)
			printf("%02x", (unsigned char)sha1[i]);
}
/**
 * This is the thread responsible for scanning periodically
 * the directory for file changes and send the appropriate
 * broadcast messages
 */ 
void * scan_for_file_changes_thread(void * time_interval){
	struct dir_files_status_list * currentDir, *currTmp, *watchedTmp, *swap, * result;
	int msglen, listlen;
	int64_t clk;
	int dirChangedFlag=0;
	while(1){
	
		printf("\n=>Dir Thread is here!! \n");
		dirChangedFlag=0;
		/*
		 * Send A UDP Status_MSG, still under development!
		 
		msglen = udp_packet_encode(STATUS_MSG,client_name,TCP_PORT,time(NULL));
		udp_packet_send(msglen);
		*/
		currentDir = listWatchedDir(watched_dir);
		currTmp = currentDir;
		watchedTmp = watched_files;
		time((time_t *)&clk);
		while(watchedTmp != NULL){
			
			/* End of list case, deleted file */
			if(currTmp == NULL){
				while(watchedTmp){
					printf("File %s deleted \n", watchedTmp->filename);
					/* Send a file Deleted message!!! */
					msglen = udp_file_packet_encode(FILE_DELETED_MSG,client_name,TCP_PORT,&clk,&watchedTmp->modifictation_time_from_epoch, watchedTmp->filename,watchedTmp->sha1sum,watchedTmp->size_in_bytes);
					udp_packet_send(msglen);	
					watchedTmp = watchedTmp->next;
					dirChangedFlag = 1;
				}
				break;
			}
			/*Same files */
			else if( (strcmp(watchedTmp->filename, currTmp->filename) ==0) ){
				/* Case Modified file */
				if( (compare_sha1(watchedTmp->sha1sum, currTmp->sha1sum) !=0 ) ||
							((watchedTmp->size_in_bytes - currTmp->size_in_bytes) !=0) ){
					printf("File %s modified \n", watchedTmp->filename);
					/* Send a Modified File message!!! */
					msglen = udp_file_packet_encode(FILE_CHANGED_MSG,client_name,TCP_PORT,&clk,&currTmp->modifictation_time_from_epoch, currTmp->filename,currTmp->sha1sum,currTmp->size_in_bytes);
					udp_packet_send(msglen);
					dirChangedFlag = 1;
				}
				watchedTmp = watchedTmp->next;
				currTmp = currTmp->next;
			}
			
			/* Different files */
			else{
				SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, currentDir, watchedTmp, ILIST_COMPARATOR, next, result);
				if(result == NULL){
					printf("File %s changed -> Deleted! \n",watchedTmp->filename);
					/* Send a file Deleted message!!! */
					msglen = udp_file_packet_encode(FILE_DELETED_MSG,client_name,TCP_PORT,&clk,&watchedTmp->modifictation_time_from_epoch, watchedTmp->filename,watchedTmp->sha1sum,watchedTmp->size_in_bytes);
					udp_packet_send(msglen);					
					watchedTmp = watchedTmp->next;
					dirChangedFlag = 1;
					
				}
				else{
					printf("File %s changed -> Added! \n",currTmp->filename);
					/* Send a file Added message!!! */
					msglen = udp_file_packet_encode(NEW_FILE_MSG,client_name,TCP_PORT,&clk,&currTmp->modifictation_time_from_epoch, currTmp->filename,currTmp->sha1sum,currTmp->size_in_bytes);
					udp_packet_send(msglen);
					currTmp = currTmp->next;
					dirChangedFlag = 1;
				}
			
			}
			
		}
		/* If got more, file added! */
		while(currTmp != NULL){
			printf("File %s Added \n",currTmp->filename);
			time((time_t *)&clk);
			/* Send a file Added mesage!!! */
			msglen = udp_file_packet_encode(NEW_FILE_MSG,client_name,TCP_PORT,&clk,&currTmp->modifictation_time_from_epoch, currTmp->filename,currTmp->sha1sum,currTmp->size_in_bytes);
			udp_packet_send(msglen);
			currTmp = currTmp->next;
			dirChangedFlag = 1;
		}
		swap = watched_files; 
		
		pthread_mutex_lock(&file_list_mutex);
		watched_files = currentDir;
		pthread_mutex_unlock(&file_list_mutex);
		
		dir_list_free(swap);
		/* Check empty list Case */
		SGLIB_LIST_LEN(struct dir_files_status_list,watched_files,next, listlen);
		if( listlen == 0 ){
			
			pthread_mutex_lock(&print_mutex);
			printf("-> The current watched Files List is EMPTY!\n");
			pthread_mutex_unlock(&print_mutex);
			
			time((time_t *)&clk);
			msglen = udp_packet_encode(DIR_EMPTY,client_name,TCP_PORT,&clk);
			udp_packet_send(msglen);
		}
		else if(dirChangedFlag == 0){
			pthread_mutex_lock(&print_mutex);
			printf("-> No changes under the current dir!\n");
			pthread_mutex_unlock(&print_mutex);
			
			time((time_t *)&clk);
			msglen = udp_packet_encode(NO_CHANGES_MSG,client_name,TCP_PORT,&clk);
			udp_packet_send(msglen);
		}
		PrintWatchedDir(watched_files);
		
		sleep((intptr_t)time_interval);
	}


}


/**
 * Free a list of  dir_files_status_list elements
 */
void dir_list_free(struct dir_files_status_list * dirList){
	dir_files_status_list *l;
	
	UNUSED(l);
	//printf("-> Trying to Free memory . .\n");
	SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, dirList, l, next, {
			free(l->filename);
			/*if(l->owner)
				free(l->owner);
			if(l->group)
				free(l->group);*/
			free(l);
	});
}

/** 
 * 	 Prints the Watched Files Sorted-List
 */
void PrintWatchedDir(dir_files_status_list * dirList){
	dir_files_status_list *l;
	char datestring[256];
	struct tm time;
	
	pthread_mutex_lock(&print_mutex);
	/* print the sorted list 
	printf("-> Sorting the watched Files List . .\n");
	SGLIB_LIST_SORT(struct dir_files_status_list, dirList, ILIST_COMPARATOR, next); */
	UNUSED(l);
	printf("-> Printing the watched Files List . .\n");
	printf("Mode\t   Owner  Group\t Size\tDate\t\t    Filename\t\t\tSHA1 \n");
	SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, dirList, l, next, {
		localtime_r(((const time_t *)&l->modifictation_time_from_epoch), &time);
        /* Get localized date string. */
        strftime(datestring, sizeof(datestring), "%F %T", &time);
		printf( (S_ISDIR(l->permission)) ?  "d" : "-");
		printf( (l->permission & S_IRUSR) ? "r" : "-");
		printf( (l->permission & S_IWUSR) ? "w" : "-");
		printf( (l->permission & S_IXUSR) ? "x" : "-");
		printf( (l->permission & S_IRGRP) ? "r" : "-");
		printf( (l->permission & S_IWGRP) ? "w" : "-");
		printf( (l->permission & S_IXGRP) ? "x" : "-");
		printf( (l->permission & S_IROTH) ? "r" : "-");
		printf( (l->permission & S_IWOTH) ? "w" : "-");
		printf( (l->permission & S_IXOTH) ? "x" : "-");
		printf(" %s %s %5jd %s %s \t", l->owner, l->group, (intmax_t) l->size_in_bytes, datestring, l->filename);
		print_sha1(l->sha1sum);
		printf(" \n");
	});
	
	pthread_mutex_lock(&stats_mutex);
	printf("\n---== Printing CloudBox Statistics ==---\n");
	printf("Broadcast messages received:\t\t%d\n",appStats.msg_num);
	printf("Messages in KiloBytes received:\t\t%f\n", (appStats.msg_size/(double)1000));
	printf("Files in KiloBytes received:\t\t%f\n", (appStats.file_size/(double)1000));
	printf("Total Transfer time:\t\t\t%f(ms)\n",appStats.total_time);
	if(appStats.file_size > 0)
		printf("Average speed in KiloBytes/sec:\t\t%f\n", ((appStats.file_size/(double)1000)/(appStats.total_time/(double)1000)));
	else
		printf("Average speed in KiloBytes/sec:\t\t~\n");
	pthread_mutex_unlock(&stats_mutex);
	
	
	
    pthread_mutex_unlock(&print_mutex);
}

/** 
 * 	Filters the results from scandir and removes hidden files
*/
static int
filter (const struct dirent *dir)
{
	if(dir->d_name[0] != '.')
		return 1;
	else
		return 0;
}

/**
 *	Scans the given directory (parameter) and stores the results in a sorted list
*/ 
dir_files_status_list * listWatchedDir(char * mydir){
  int count,i;
  struct direct **files;
  struct stat statbuf;
  struct passwd pwent;
  struct passwd *pwentp;
  struct group grp;
  struct group *grpt;
  char buf[256];
  char * fullpath;
  struct dir_files_status_list * tmp;
  struct dir_files_status_list * dirList;
  
  count = scandir(mydir, &files, filter, alphasort);
  dirList = NULL;
  if(count > 0)
  {
    printf("total files:%d ~ Cloudbox directory: %s \n",count, mydir);
	pthread_mutex_lock(&file_list_mutex);
	
    for (i=0; i<count; ++i)
    {
	  tmp = (dir_files_status_list *) malloc (sizeof(struct dir_files_status_list)); 
	  fullpath = malloc(strlen(mydir) + strlen(files[i]->d_name) + 1);
      if (!fullpath || !tmp) {
        fprintf(stderr, "malloc() failed: insufficient memory!\n");
        exit(EXIT_FAILURE);
	  }
	  /*Concat path + filename to get the correct stats */
	  strcpy(fullpath, mydir);
      strcat(fullpath, files[i]->d_name);
	  
      if (stat(fullpath, &statbuf) == 0)
      {

        if (!getpwuid_r(statbuf.st_uid, &pwent, buf, sizeof(buf), &pwentp)){
		  tmp->owner = (char * ) malloc ( strlen(pwent.pw_name));
		  if (!tmp->owner) {
			fprintf(stderr, "malloc() failed: insufficient memory!\n");
			exit(EXIT_FAILURE);
		  }
		  sprintf(tmp->owner, "%s", pwent.pw_name);
		}  
        else{
		  tmp->owner = (char * ) malloc ( sizeof(statbuf.st_uid));
		  if (!tmp->owner) {
			fprintf(stderr, "malloc() failed: insufficient memory!\n");
			exit(EXIT_FAILURE);
		  }
		  sprintf(tmp->owner, "%d", statbuf.st_uid);
		}
		  
        if (!getgrgid_r (statbuf.st_gid, &grp, buf, sizeof(buf), &grpt)){
		  tmp->group = (char *) malloc (strlen(grp.gr_name));
		  if (!tmp->group) {
			fprintf(stderr, "malloc() failed: insufficient memory!\n");
			exit(EXIT_FAILURE);
		  }
		  sprintf(tmp->group, "%s", grp.gr_name);
		}
        else{
		  tmp->group = (char *) malloc (sizeof(statbuf.st_gid));
		  if (!tmp->group) {
			fprintf(stderr, "malloc() failed: insufficient memory!\n");
			exit(EXIT_FAILURE);
		  }
		  sprintf(tmp->group, "%d", statbuf.st_gid);
		}
		
		/* Copy to List */
		tmp->filename = strdup(files[i]->d_name);
		if (!tmp->filename) {
			fprintf(stderr, "malloc() failed: insufficient memory!\n");
			exit(EXIT_FAILURE);
		  }
		tmp->size_in_bytes = statbuf.st_size;
		tmp->modifictation_time_from_epoch = statbuf.st_mtime;
		tmp->permission = statbuf.st_mode;
		tmp->processed = FALSE;
		compute_sha1_of_file(tmp->sha1sum, fullpath);
		SGLIB_SORTED_LIST_ADD(struct dir_files_status_list, dirList, tmp, ILIST_COMPARATOR, next);
		
      }
	  
	  
      free (files[i]);
	  free(fullpath);
    }
	pthread_mutex_unlock(&file_list_mutex);

    free(files);
  }
  return dirList;

}

int
main(int argc, char **argv){
	int opt;
	int scan_interval;
	
	/*
	 * Define the threads needed in our case
	 */
	pthread_t dir_thread, udp_thread, tcp_thread;
	int t1, t2, t3;

	/*
	 * Initialize the mutexes
	 */
	pthread_mutex_init(&print_mutex, NULL);
	pthread_mutex_init(&file_list_mutex, NULL);
	 
	/* Initialize the List Head */
	watched_files = NULL;
	/* Initialise stats */
	appStats.msg_num=0;appStats.msg_size=0;
	appStats.file_size=0;appStats.total_time=0;
	
	/* Initialize the TCP Sever Thread! */
	if( (t3 = pthread_create( &tcp_thread, NULL, handle_incoming_tcp_connection_thread, (void *) (intptr_t) 1) )){
					fprintf(stderr,"TCP Sever Thread creation failed: %d\n", t3);
					exit(EXIT_FAILURE);
	}
	
	while ((opt = getopt(argc, argv, "hn:d:i:b:")) != -1) {
		switch(opt){
			case 'n':
				client_name = (char *) strdup(optarg);
				break;
				
			case 'd':
				/* A few checks will be nice here...*/
				/* Convert the given dir to absolute path */
				watched_dir = strdup(optarg);
				if((watched_dir == NULL) || strcmp(watched_dir, " ") == 0){
					fprintf(stderr, "Wrong Dir Name input!\n");
					return EXIT_FAILURE;	
				}
				else if(strcmp(watched_dir, ".") ==0 ){
					watched_dir = "./";
				}
				else if(watched_dir[strlen(watched_dir)-1] != '/'){
					watched_dir = realloc(watched_dir , strlen(watched_dir) + 1);
					strcat(watched_dir,"/");
				}
				watched_files = listWatchedDir(watched_dir);
				PrintWatchedDir(watched_files);
				break;
			case 'i':
				scan_interval = atoi(optarg);
				/* I thought the interval was in minutes but it seems its in seconds 
				scan_interval *= 60;*/
				
				if( (t1 = pthread_create( &dir_thread, NULL, scan_for_file_changes_thread, (void *) (intptr_t) scan_interval) )){
					fprintf(stderr,"Dir_Thread creation failed: %d\n", t1);
					exit(EXIT_FAILURE);
				}
				//pthread_join(dir_thread, NULL);
				
				break;
			case 'b':
				broadcast_port = atoi(optarg);
				if(!((broadcast_port > 1023) || (broadcast_port <65536))){
					fprintf(stderr, "Not a Valid Broadcast Port to bind!\n");
					return EXIT_FAILURE;
				}
				if( (t2 = pthread_create( &udp_thread, NULL, udp_receiver_dispatcher_thread, (void *) (intptr_t) broadcast_port) )){
					fprintf(stderr,"Dir_Thread creation failed: %d\n", t2);
					exit(EXIT_FAILURE);
				}
				pthread_join(udp_thread, NULL);
				
				break;
			default:
				printf("Usage: cloudbox -n client_name -d directory_to_use -i scan_interval -b broadcast_port\n"
				"Options:\n"
				"   -n                  Specifies the name of the client\n"
				"   -d                  The directory absolute path, to watch for changes\n"
				"   -i                  The interval time in seconds, that the client should scan for file changes\n"
				"   -b                  The port that is going to be used for receiving and transmitting broadcasts UDP meesages\n"
				"   -h                  prints this help\n");
				exit(EXIT_FAILURE);
		}
	}
	
	
	/*  
	 * Destroy the mutexes
	 */
     pthread_mutex_destroy(&print_mutex);
	 pthread_mutex_destroy(&file_list_mutex);
	
	
	
	printf("\n Cloudbox client %s:\n"
		   "Watched directory: %s\n"
		   "Scan interval: %d minutes\n"
		   "Broadcast port: %d\n",
		client_name, watched_dir, (scan_interval/60), broadcast_port);
	
	
	
	return 0;
}
