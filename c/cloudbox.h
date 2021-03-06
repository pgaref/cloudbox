/**
 * @file cloudbox.h
 * @author Surligas Manos surligas@csd.uoc.gr
 * 
 * This is the header file for the Cloudbox client.
 * 
 * There are many function definitions here, that you
 * have to implement.
 * 
 * You can NOT alter the contracts of these functions.
 * 
 * You are FREE to add your own functions, structs,
 * etc.
 * 
 * If you find a bug report it ASAP
 * 
 * Have fun!
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <utime.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <getopt.h>
#include <stdint.h>
/*
 * NOTE: pthread.h needs -lpthread flag during compilation.
 */
#include <pthread.h>
#include <stddef.h>
/*
 * For SHA1 computation.
 * NOTE: You have to install the development files for openssl
 * at your computer and compile against libssl and libcrypto.
 * See the Makefile for details
 */
#include <openssl/sha.h>

/**
 *  Socket Libraries and dependencies
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/**
 * An easy way to support boolean type in C
 */
typedef char boolean;

#define TRUE 1
#define FALSE 0

/**
 * This is the length of the SHA1 output in bytes.
 * SHA1 is 160 bit long, equals to 20 bytes
 */
#define SHA1_BYTES_LEN 20
/**
 * Maximum size of a datagram in most of the cases! 
 */
#define MAXBUF 65535

/* 
 * SGLIB - data structures
 */
#include "sglib.h"

#define UNUSED(x) (void)(x)
#define ILIST_COMPARATOR(e1, e2) (strcasecmp(e1->filename, e2->filename))

/**
 * An enum type indicating the type of each message
 * send by the client.
 * 
 * Message type should consist the TWO FIRST BYTES
 * of each packet if necessary. It helps the receiver
 * to figure out what to do.
 * 
 * NOTE: By default enum types are integers so they
 * are four bytes. Take care of it and cast
 * the message type to a proper 2-byte type.
 */
typedef enum {
	STATUS_MSG = 0x1, 				/**< A status message */
	NO_CHANGES_MSG = 0x2, 			/**< Client's directory is clean */
	NEW_FILE_MSG = 0x3, 			/**< A new file added at the client */
	FILE_CHANGED_MSG = 0x4, 		/**< A file changed at the client */
	FILE_DELETED_MSG = 0x5, 		/**< A file deleted at the client */
	FILE_TRANSFER_REQUEST = 0x6,	/**< Another client wants our file */
	FILE_TRANSFER_OFFER = 0x7,		/**< Another client sends us a file */
	DIR_EMPTY = 0x08,				/**< The directory of the client has no files */
	NOP = 0xff						/**< Do nothing. Use it for debug reasons */
} msg_type_t;


typedef struct cloudbox_stats{
	int 	msg_num;
	double 	msg_size; /* in bytes */
	double 	file_size; /* in bytes */
	double	total_time; /* in ms */
}cloudboxStats;

/**
 * A linked list (double or single, choose what you want)
 * that keeps appropriate info for each file at the 
 * directory that the client scans for changes.
 * 
 * NOTE: Feel free to add your own fields
 */
typedef struct dir_files_status_list {
	char *	filename;
	off_t 	size_in_bytes;
	char 	sha1sum[SHA1_BYTES_LEN];
	int64_t modifictation_time_from_epoch;
	/* Extra fields! */
	char  * owner;
	char  * group;
	mode_t  permission;
	boolean processed;
	struct dir_files_status_list *next;
} dir_files_status_list;

/*
 * The list that holds all the current watched files.
 * 
 * It is very convinient this list to be shorted by the file name
 * in order to be able to find immediatly inconsistencies,
 */
extern struct dir_files_status_list * watched_files;
extern struct cloudbox_stats appStats;
extern char * watched_dir;
extern int broadcast_port;
extern char *client_name;
extern int TCP_PORT;

dir_files_status_list * listWatchedDir(char * );
void PrintWatchedDir(dir_files_status_list * );
void dir_list_free(struct dir_files_status_list *);
/*
 * Print mutex, for printing nicely the messages from different threads
 */
extern pthread_mutex_t print_mutex;

extern pthread_mutex_t stats_mutex;
/* 
 * Mutex used to protect the accesses from different threads
 * of the file list of the watched directory
 */
extern pthread_mutex_t file_list_mutex;

/**
 * Computes the SHA1 checksum of a file.
 * For convinience use the SHA1(char *, size_t, char *)
 * function.
 * 
 * @param filename The name of the file that we want
 * to compute the SHA1 checksum
 * 
 * @param outbuff The buffer in which the SHA1 checksum
 * shall be stored. outbuff should be SHA1_BYTES_LEN bytes
 * long.
 */
void compute_sha1_of_file(char *outbuff, char *filename);

/**
 * Computes the SHA1 checksum of a buffer.
 * 
 * For convinience use the SHA1(char *, size_t, char *)
 * function. 
 * 
 * @param outbuff The buffer in which the SHA1 checksum
 * shall be stored. outbuff should be SHA1_BYTES_LEN bytes
 * long.
 * 
 * @param buffer The buffer contaning the data
 * @param buffer_len The length of the data in bytes
 */
void compute_sha1_of_buffer(char *outbuff, char *buffer, size_t buffer_len);


/**
 * This is the thread responsible for scanning periodically
 * the directory for file changes and send the appropriate
 * broadcast messages
 * 
 * @param @params The structure casted to (void *) containing
 * all the needed parameters. Add your parameters list at your
 * own will.
 */ 
void * scan_for_file_changes_thread(void * time_interval);

/**
 * Thread that handles a new incoming TCP connection.
 * 
 * @param params The structure casted to (void *) containing
 * all the needed parameters. Add your parameters list at your
 * own will. Of course one of these parameters should be these
 * socket descriptor that accept() call returned.
 */
void * handle_incoming_tcp_connection_thread(void *params);


/**
 * Thread that receives UDP broadcast messages and
 * based on the message type, the dispatcher 
 * can fire up a new thread or do a specific job.
 */
void * udp_receiver_dispatcher_thread(void *params);
/**
 * Encode a packet according to the fields and send it!
 */
void udp_packet_encodeNsend();
/**
 * Encode a packet according to the fields
 */
int udp_packet_encode(msg_type_t type, char * client_name, int tcp_port, int64_t *mod_time);
/**
 * Encode a complex file packet according to the fields
 */
int udp_file_packet_encode(msg_type_t type, char * client_name, int tcp_port, int64_t *curr_time, int64_t *mod_time, char * filename, char *sha,off_t file_size);
/**
 * Send a udp packet !
 */
void udp_packet_send(int buflen);
/**
 * Extract the client name from the udp packet data
 */
char * udp_packet_clientName(char * packet);
/**
 * Send a file through TCP
 */
int send_file(char * client_ip, uint16_t port, char * filename);
/**
 * A fuction to compare two sha hashes byte - byte comparison. Returns 0 if they are equal, 1 otherwise.
 */
int compare_sha1(char * sha1, char * sha2);
/**
 * Prints a readable representation of SHA hash
 */
void print_sha1(char * sha1);

/*
 * Change file stats after a transfer!
 */
 void change_file_stats(char * filename, int64_t modtime);