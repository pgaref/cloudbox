
cloudbox: cloudbox.h cloudbox.c sglib.h tcp_server.c tcp_client.c udp_client.c
	gcc cloudbox.c udp_client.c tcp_server.c tcp_client.c -g -Wall -o cloudbox -lpthread -lcrypto -lssl -D_FILE_OFFSET_BITS=64

clean:
	rm cloudbox
