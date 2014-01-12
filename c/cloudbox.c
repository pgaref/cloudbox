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

/**
 * Computes the SHA1 checksum of a file.
 */
void compute_sha1_of_file(char *outbuff, char *filename){
	/* The data lenght to be hashed */
	size_t length = sizeof(filename);
	
	SHA1((const unsigned char *)filename, length, (unsigned char *) outbuff);
	/* outbuff now contains the 20-byte SHA-1 hash */
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
	struct dir_files_status_list * currentDir, *l;
	UNUSED(l);
	while(1){
	
		printf("\n Dir Thread is here!! \n");
		
		currentDir = listWatchedDir(watched_dir);
		SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, watched_files, l, next, {
		
			/*Same files */
			if(strcmp(l->filename, currentDir->filename) ==0 ){
				/* Case Modified file */
				if(currentDir->modifictation_time_from_epoch > l->modifictation_time_from_epoch){
					printf("File %s modified \n", l->filename);
				}
				
			}
			
			/* Different files */
			else{
				printf("File %s changed! \n",l->filename);
			
			}
			currentDir = currentDir->next;
		})
		
		sleep((intptr_t)time_interval);
	}


}

/** 
 * 	 Prints the Watched Files Sorted-List
 */
void PrintWatchedDir(dir_files_status_list * dirList){
	dir_files_status_list *l;
	char datestring[256];
	struct tm time;
	
	/* print the sorted list 
	printf("-> Sorting the watched Files List . .\n");
	SGLIB_LIST_SORT(struct dir_files_status_list, dirList, ILIST_COMPARATOR, next); */
	UNUSED(l);
	printf("-> Printing the watched Files List . .\n");
	printf("Mode\t   Owner  Group\t Size\tDate\t\t    Filename \n");
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
		printf(" %s %s %5jd %s %s \n", l->owner, l->group, l->size_in_bytes, datestring, l->filename);
	})

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
	int broadcast_port;
	char *client_name;
	
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
				pthread_join(dir_thread, NULL);
				
				break;
			case 'b':
				broadcast_port = atoi(optarg);
				/* To check or not to check? */
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
