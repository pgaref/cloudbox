#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <getopt.h>

#include "cloudbox.h"
#include "sglib.h"

/*
 * The list that holds all the current watched files.
 * 
 * It is very convinient this list to be shorted by the file name
 * in order to be able to find immediatly inconsistencies,
 */
struct dir_files_status_list * watched_files;
extern int alphasort();

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


/* We dont care about hidden files */
static int
filter (const struct dirent *dir)
{
	if(dir->d_name[0] != '.')
		return 1;
	else
		return 0;
}

void listWatchedDir(char * mydir){
  int count,i;
  struct direct **files;
  struct stat statbuf;
  char datestring[256];
  struct passwd pwent;
  struct passwd *pwentp;
  struct group grp;
  struct group *grpt;
  struct tm time;
  char buf[1024];
  char * fullpath;
  struct dir_files_status_list * tmp;

  printf("~~~ pgaref Starts Here! \n");
  count = scandir(mydir, &files, filter, alphasort);

  if(count > 0)
  {
    printf("total files:%d ~ Cloudbox directory: %s \n",count, mydir);
	
    for (i=0; i<count; ++i)
    {
	  tmp = (dir_files_status_list *) malloc (sizeof(struct dir_files_status_list)); 
	  fullpath = malloc(strlen(mydir) + strlen(files[i]->d_name) + 1);
      if (!fullpath) {
        fprintf(stderr, "malloc() failed: insufficient memory!\n");
        exit(EXIT_FAILURE);
	  }
	  /*Concat path + filname to get the correct stats */
	  strcpy(fullpath, mydir);
      strcat(fullpath, files[i]->d_name);
	  
      if (stat(fullpath, &statbuf) == 0)
      {
		
        /* Print out type, permissions, and number of links. */
        printf(" %d", statbuf.st_nlink);

        if (!getpwuid_r(statbuf.st_uid, &pwent, buf, sizeof(buf), &pwentp)){
          printf(" %s", pwent.pw_name);
		  sprintf(tmp->owner, "%s", pwent.pw_name);
		}  
        else{
          printf(" %d", statbuf.st_uid);
		  sprintf(tmp->owner, "%d", statbuf.st_uid);
		}
		  
        if (!getgrgid_r (statbuf.st_gid, &grp, buf, sizeof(buf), &grpt)){
          printf(" %s", grp.gr_name);
		  sprintf(tmp->group, "%s", grp.gr_name);
		}
        else{
          printf(" %d", statbuf.st_gid);
		  sprintf(tmp->group, "%d", statbuf.st_gid);
		}
        /* Print size of file. */
        printf(" %5d", (int)statbuf.st_size);

        localtime_r(&statbuf.st_mtime, &time);
        /* Get localized date string. */
        strftime(datestring, sizeof(datestring), "%F %T", &time);
        printf(" %s %s \n", datestring, files[i]->d_name);
		
		/* Copy to List */
		tmp->filename = fullpath;
		tmp->size_in_bytes = statbuf.st_size;
		tmp->modifictation_time_from_epoch = statbuf.st_mtime;
		SGLIB_LIST_ADD(struct dir_files_status_list, watched_files, tmp, next);
      }

      free (files[i]);
    }

    free(files);
  }
  printf("\n ~~~ pgaref Ends Here! ~~~ \n");

}


int
main(int argc, char **argv){
	int opt;
	int scan_interval;
	int broadcast_port;
	
	char *client_name;
	char *watched_dir;
	char * tmp;
	dir_files_status_list *l;
	
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
				client_name = strdup(optarg);
				break;
				
			case 'd':
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
				listWatchedDir(watched_dir);
				/* A few checks will be nice here...*/
				/* Convert the given dir to absolute path */
				break;
			case 'i':
				scan_interval = atoi(optarg);
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
	
	printf("Cloudbox client %s:\n"
		   "Watched directory: %s\n"
		   "Scan interval: %d seconds\n"
		   "Broadcast port: %d\n",
		client_name, watched_dir, scan_interval, broadcast_port);
	
	/* sort & then print the list */
	printf("-> Sorting the watched Files List!\n");
	SGLIB_LIST_SORT(struct dir_files_status_list, watched_files, ILIST_COMPARATOR, next);
	SGLIB_LIST_MAP_ON_ELEMENTS(struct dir_files_status_list, watched_files, l, next, {
		printf("Element: %s \n", l->filename);
	})
	
	return 0;
}
