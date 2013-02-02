
/*********************
 * UDP RECEIVER TEST 3
 * ******************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strncpy */
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define MIN( x, y ) ( x < y ? x : y )


ssize_t recvfrom_large( int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen ) {
  int sum_size = 0;
  int max_tcp_message_size = 65500;

  //P_DEBUG( 2, "recvfrom_large : receiving " << len << endl );
  while ( sum_size < len ) {
    int recvd = recvfrom( sockfd, &( (char *)buf )[ sum_size ], max_tcp_message_size, flags, src_addr, addrlen );
    if ( recvd <= 0 ) printf( "ERROR on recvfrom() " );
    //P_DEBUG( 2, "PARTIAL RECVD " << recvd << endl);
    sum_size += ( recvd > 0 ) * recvd;
  }
  return sum_size;
}


int main(int argc, char *argv[])
{

	if(argc < 2) {
		printf("\nUsage: ./receiver <num_sender> <first_receiver_port> <packet_size_in_KB> <packet_number>\n\n");
		exit(1);
	}

	int num_sender = atoi(argv[1]); // how many people are sending us data?
	int port[num_sender]; // ports array
	int sock[num_sender]; // socket descriptor array
	int first_port = atoi(argv[2]); // first port for the server
	int packet_size = atoi(argv[3]); // how many KB is a packet?
	int true = 1, message_size = packet_size*1024, bytes_received;
	long total_bytes_recv = 0;
	char recv_data[message_size];
	int packet_number = atoi(argv[4]); // how many packets are we going to receive?

	struct sockaddr_in server_addr[num_sender], client_addr[num_sender]; // array for server and client sockets
	int sin_size;
	int connected[num_sender]; // connection descriptor array

	// this is for the timer
	struct timeval tv;
	double receive_start, receive_end;

	int k, j, rc;
	for (k = 0; k < num_sender; k++) {
		// socket creation
		if ((sock[k] = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("Socket");
			exit(1);
		}

		// socket options setting
		if (setsockopt(sock[k], SOL_SOCKET, SO_REUSEADDR, &true,sizeof(int)) == -1) {
			perror("Setsockopt SO_REUSEADDR");
			exit(1);
		}

		/*
		if (setsockopt(sock[k], IPPROTO_TCP, TCP_NODELAY, &true, sizeof(int)) == -1)
		{
			perror("Setsockopt TCP_NODELAY");
			exit(1);
		}
		*/

		/*
		   rc = ioctl(sock[k], FIONBIO, &true);
		   if (rc < 0)
		   {
		   perror("ioctl() failed");
		   close(sock[k]);
		   exit(-1);
		   }
		   */
	}

	for (k = 0; k < num_sender; k++) {
		// sockaddr settings
		server_addr[k].sin_family = AF_INET;         
		server_addr[k].sin_port = htons(first_port + k);
		server_addr[k].sin_addr.s_addr = INADDR_ANY; 
		bzero(&(server_addr[k].sin_zero),sizeof(server_addr[k].sin_zero));

		// socket binding
		if (bind(sock[k], (struct sockaddr_in *)&server_addr[k], sizeof(server_addr[k])) == -1) {
			perror("Unable to bind");
			exit(1);
		}

		/* listening on socket
		if(listen(sock[k], 1) == -1) {
			perror("Listen");
			exit(1);
		}

		printf("Listening on port %d\n", (first_port + k));
		*/
	}

	sin_size = sizeof(struct sockaddr_in);
	double diff_time, total_time = 0;
	
	/* accepting incoming connections
	for (k = 0; k < num_sender; k++) {
		connected[k] = accept(sock[k], (struct sockaddr *)&client_addr[k], &sin_size);
		printf("Received connection from %d\n", k);
	}
	*/

	// receiving data and measuring the time
	int partial_bytes;
	double partial_time;
	gettimeofday(&tv, NULL);
	receive_start = tv.tv_sec + (tv.tv_usec/1000000.0);
	for (k = 0; k < packet_number; k++) {
		for (j = 0; j < num_sender; j++) {
			//bytes_received = recvfrom(sock[j], recv_data, message_size, 0,(struct sockaddr_in *) &client_addr[k], &sin_size	);
			
			bytes_received = recvfrom_large(sock[j], recv_data, message_size, 0, (struct sockaddr_in *) &client_addr[k], &sin_size );

			
			total_bytes_recv += bytes_received;

			printf("Partial received %d bytes\n", bytes_received);
		}
	}
	gettimeofday(&tv, NULL);
	receive_end = tv.tv_sec + (tv.tv_usec/1000000.0);
	total_time = receive_end - receive_start;

	// closing sockets
	for (k = 0; k < num_sender; k++)
		close(sock[k]);

	printf("Total bytes received %ld in %f usec\n", total_bytes_recv, total_time*1000000);
	printf("Bandwidth: %f Gb/s\n", (total_bytes_recv*8)/(1024*1024*1024*total_time));

	fflush(stdout);

	return 0;
}

