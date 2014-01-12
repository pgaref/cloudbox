#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cloudbox.h"

int
main(int argc, char **argv){
	int opt;
	int scan_interval;
	int broadcast_port;
	
	char *client_name;
	char *watched_dir;
	
	while ((opt = getopt(argc, argv, "hn:d:i:b:")) != -1) {
		switch(opt){
			case 'n':
				client_name = strdup(optarg);
				break;
				
			case 'd':
				watched_dir = strdup(optarg);
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
	"Wathced directory: %s\n"
	"Scan interval: %d seconds\n"
	"Broadcast port: %d\n",
	client_name, watched_dir, scan_interval, broadcast_port);
	
	return 0;
}