/*
 * NOTE: pthread.h needs -lpthread flag during compilation.
 */
#include <pthread.h>
#include <stddef.h>
#include <string>
#include <vector>
/*
 * For SHA1 computation.
 * NOTE: You have to install the development files for openssl
 * at your computer and compile against libssl and libcrypto.
 * See the Makefile for details
 */
#include <openssl/sha.h>

/**
 * This is the length of the SHA1 output in bytes.
 * SHA1 is 160 bit long, equals to 20 bytes
 */
#define SHA1_BYTES_LEN 20

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


/**
 * A list that keeps appropriate info for each file at the 
 * directory that the client scans for changes.
 * 
 * NOTE: Feel free to add your own fields
 */
struct dir_files_status_list {
	char *filename;
	size_t size_in_bytes;
	char sha1sum[SHA1_BYTES_LEN];
	long long int modifictation_time_from_epoch;
};

class cloudbox
{
public:
	cloudbox(const std::string &client_name,
			 const std::string &watched_dir,
		     const unsigned short broadcast_port,
		     const int interval);
	
	~cloudbox();
	
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
	static void compute_sha1_of_file(char *outbuff, char *filename);
	
	
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
	static void compute_sha1_of_buffer(char *outbuff, char *buffer, size_t buffer_len);
	
	/**
	 * This is the thread responsible for scanning periodically
	 * the directory for file changes and send the appropriate
	 * broadcast messages
	 * 
	 * @param @params The structure casted to (void *) containing
	 * all the needed parameters. Add your parameters list at your
	 * own will.
	 */ 
	static void scan_for_file_changes_thread(void *params);
	
	/**
	 * Thread that handles a new incoming TCP connection.
	 * 
	 * @param params The structure casted to (void *) containing
	 * all the needed parameters. Add your parameters list at your
	 * own will. Of course one of these parameters should be these
	 * socket descriptor that accept() call returned.
	 */
	static void handle_incoming_tcp_connection_thread(void *params);
	
	
	/**
	 * Thread that receives UDP broadcast messages and
	 * based on the message type, the dispatcher 
	 * can fire up a new thread or do a specific job.
	 */
	static void udp_receiver_dispatcher_thread(void *params);
	
private:
	std::string										d_client_name;
	std::string										d_watched_dir;
	
	unsigned short 									d_broadcast_port;
	int												d_scan_interval;
	
	std::vector<struct dir_files_status_list> 		d_whatched_files;
	
	/**
	 * Print mutex, for printing nicely the messages from different threads
	 */
	pthread_mutex_t 								d_print_mutex;
	
	/** 
	 * Mutex used to protect the accesses from different threads
	 * of the file list of the watched directory
	 */
	pthread_mutex_t 								d_file_list_mutex;
	
	/* Feel free to add your own priavte members */
};
