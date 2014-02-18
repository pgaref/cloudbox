#include "cloudbox.h"
#include "sglib.h"
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


#define UNUSED(x) (void)(x)
#define ILIST_COMPARATOR(e1, e2) (strcasecmp(e1->filename, e2->filename))
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
 * Decode UDP packet according to the given instructions!
 */
void udp_packet_decode(char * packet){
	char pak[MAXBUF];
	time_t clk;
	char client_name[255];
	uint16_t tcp_port;
	int count =3 , i=0;
	char tmp[2];
	
	memcpy(pak, packet,MAXBUF);
	tmp[0] = pak[0];
	tmp[1] = pak[1];
	pthread_mutex_lock(&print_mutex);
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
	
	printf("\tClient Name: %s\n", client_name);
	printf("\tTCP Listening Port: %u \n", tcp_port);
	printf("\tPacket Sent at: %s \n", ctime(&clk));
	/*
	printf("File modification Time %s \n",ctime(&mod_time));
	*/
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
	sock_in.sin_port = htons(5555);
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
		udp_packet_decode(buffer);
		if(!strcmp(buffer, "quit")){
			printf("Quiting. . .\n");
			break;
		}
		pthread_mutex_lock(&print_mutex);
		printf("UDP SERVER: read %d bytes from IP %s:%d(%s)\n", status,
		       inet_ntoa(sock_in.sin_addr),sock_in.sin_port, buffer);
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
/**
 * This is the thread responsible for scanning periodically
 * the directory for file changes and send the appropriate
 * broadcast messages
 */ 
void * scan_for_file_changes_thread(void * time_interval){
	struct dir_files_status_list * currentDir, *currTmp, *watchedTmp, *swap, * result;
	int msglen;
	while(1){
	
		printf("\n=>Dir Thread is here!! \n");
		
		/*
		 * Send A UDP Status_MSG, still under development!
		 */
		msglen = udp_packet_encode(STATUS_MSG,client_name,TCP_PORT,time(NULL));
		udp_packet_send(msglen);
		
		currentDir = listWatchedDir(watched_dir);
		currTmp = currentDir;
		watchedTmp = watched_files;
		
		while(watchedTmp != NULL){
			
			/* End of list case, deleted file */
			if(currTmp == NULL){
				while(watchedTmp){
					printf("File %s deleted \n", watchedTmp->filename);
					watchedTmp = watchedTmp->next;
				}
				break;
			}
			/*Same files */
			else if( (strcmp(watchedTmp->filename, currTmp->filename) ==0) ){
				/* Case Modified file */
				if( (currTmp->modifictation_time_from_epoch != watchedTmp->modifictation_time_from_epoch) || 
						(strncmp(watchedTmp->sha1sum, currTmp->sha1sum, 20) !=0 ) ||
							((watchedTmp->size_in_bytes - currTmp->size_in_bytes) !=0) ){
					printf("File %s modified \n", watchedTmp->filename);
				}
				watchedTmp = watchedTmp->next;
				currTmp = currTmp->next;
			}
			
			/* Different files */
			else{
				SGLIB_LIST_FIND_MEMBER(struct dir_files_status_list, currentDir, watchedTmp, ILIST_COMPARATOR, next, result)
				if(result == NULL){
					printf("File %s changed -> Deleted! \n",watchedTmp->filename);	
					watchedTmp = watchedTmp->next;
					
				}
				else{
					printf("File %s changed -> Added! \n",currTmp->filename);
					currTmp = currTmp->next;
				}
			
			}
			
		}
		/* If got more, file added! */
		while(currTmp != NULL){
			printf("File %s Added \n",currTmp->filename);
			currTmp = currTmp->next;
		}
		swap = watched_files; 
		watched_files =currentDir;
		
		dir_list_free(swap);
		PrintWatchedDir(watched_files);
		sleep((intptr_t)5);
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
			free(l->owner);
			free(l->group);
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
	int i;
	
	pthread_mutex_lock(&print_mutex);
	/* print the sorted list 
	printf("-> Sorting the watched Files List . .\n");
	SGLIB_LIST_SORT(struct dir_files_status_list, dirList, ILIST_COMPARATOR, next); */
	UNUSED(l);
	printf("-> Printing the watched Files List . .\n");
	printf("Mode\t   Owner  Group\t Size\tDate\t\t    Filename\t\t\tSHA1 \n");
	SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, dirList, l, next, {
		localtime_r((&l->modifictation_time_from_epoch), &time);
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
		for(i = 0; i < SHA1_BYTES_LEN; i++)
			printf("%02x", (unsigned char)l->sha1sum[i]);
		printf(" \n");
	});
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
  char buf[1024];
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
      if (!fullpath) {
        fprintf(stderr, "malloc() failed: insufficient memory!\n");
        exit(EXIT_FAILURE);
	  }
	  /*Concat path + filename to get the correct stats */
	  strcpy(fullpath, mydir);
      strcat(fullpath, files[i]->d_name);
	  
      if (stat(fullpath, &statbuf) == 0)
      {

        if (!getpwuid_r(statbuf.st_uid, &pwent, buf, sizeof(buf), &pwentp)){
		  tmp->owner = (char * ) malloc ( sizeof(pwent.pw_name));
		  sprintf(tmp->owner, "%s", pwent.pw_name);
		}  
        else{
		  tmp->owner = (char * ) malloc ( sizeof(statbuf.st_uid));
		  sprintf(tmp->owner, "%d", statbuf.st_uid);
		}
		  
        if (!getgrgid_r (statbuf.st_gid, &grp, buf, sizeof(buf), &grpt)){
		  tmp->group = (char *) malloc (sizeof(grp.gr_name));
		  sprintf(tmp->group, "%s", grp.gr_name);
		}
        else{
		  tmp->group = (char *) malloc (sizeof(statbuf.st_gid));
		  sprintf(tmp->group, "%d", statbuf.st_gid);
		}
		
		/* Copy to List */
		tmp->filename = fullpath;
		tmp->size_in_bytes = statbuf.st_size;
		tmp->modifictation_time_from_epoch = statbuf.st_mtime;
		tmp->permission = statbuf.st_mode;
		compute_sha1_of_file(tmp->sha1sum, tmp->filename);
		SGLIB_SORTED_LIST_ADD(struct dir_files_status_list, dirList, tmp, ILIST_COMPARATOR, next);
      }

      free (files[i]);
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
				scan_interval *= 60;
				
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
