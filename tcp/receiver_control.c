/*********************
 * TCP SOCKET TEST 3
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

int main(int argc, char *argv[])
{

	if(argc < 6) {
		printf("\nUsage: ./receiver <num_sender> <first_receiver_port> <packet_size_in_KB> <packet_number> <control_port>\n\n");
		exit(1);
	}

	int num_sender = atoi(argv[1]); // how many people are sending us data?
	int port[num_sender]; // ports array
	int sock[num_sender]; // socket descriptor array
	int first_port = atoi(argv[2]); // first port for the server
	int packet_size = atoi(argv[3]); // how many KB is a packet?
	int true = 1, bytes_received;
	long message_size = packet_size*1024;
	long total_bytes_recv = 0;
	char recv_data[message_size];
	int packet_number = atoi(argv[4]); // how many packets are we going to receive?

	struct sockaddr_in this_addr[num_sender], sender_addr[num_sender]; // array for server and client sockets
	int sin_size;
	int connected[num_sender]; // connection descriptor array

	// this is for the timer
	struct timeval tv;
	double receive_start, receive_end;

	int k, j, rc;
	for (k = 0; k < num_sender; k++) {
		// socket creation
		if ((sock[k] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
		}

		// socket options setting
		if (setsockopt(sock[k], SOL_SOCKET, SO_REUSEADDR, &true,sizeof(int)) == -1) {
			perror("Setsockopt SO_REUSEADDR");
			exit(1);
		}

		if (setsockopt(sock[k], IPPROTO_TCP, TCP_NODELAY, &true, sizeof(int)) == -1)
		{
			perror("Setsockopt TCP_NODELAY");
			exit(1);
		}

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
		this_addr[k].sin_family = AF_INET;         
		this_addr[k].sin_port = htons(first_port + k);
		this_addr[k].sin_addr.s_addr = INADDR_ANY; 
		bzero(&(this_addr[k].sin_zero),8);

		// socket binding
		if (bind(sock[k], (struct sockaddr *)&this_addr[k], sizeof(struct sockaddr)) == -1) {
			perror("Unable to bind");
			exit(1);
		}

		// listening on socket
		if(listen(sock[k], 1) == -1) {
			perror("Listen");
			exit(1);
		}

		//printf("Listening on port %d\n", (first_port + k));
	}

	sin_size = sizeof(struct sockaddr_in);
	double diff_time, total_time = 0, accept_start, accept_end, total_accept = 0;


	int control_port = atoi(argv[5]); /* DEFAULT control UDP port to receive the
							  * "start" command */
	int cmd_len = 256; /* max length of the command line received via the
						* control port */
	/* Create the socket */
	int ctrl_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( ctrl_socket < 0)
		perror( "ERROR opening control socket" );
	struct sockaddr_in ctrl_address;
	bzero( (char *) &ctrl_address, sizeof( ctrl_address ) );

	ctrl_address.sin_family = AF_INET;
	ctrl_address.sin_addr.s_addr = INADDR_ANY;
	ctrl_address.sin_port = htons( control_port );

	//P_DEBUG( 1, "LISTEN for START on port " << ntohs( ctrl_address.sin_port ) << endl );

	socklen_t tmplen = sizeof( ctrl_address );

	/* Bind the socket */
	int bind_error = bind( ctrl_socket, (struct sockaddr *) &ctrl_address, tmplen );
	if ( bind_error < 0 )
		//ERRNO( "on binding control socket" << endl );
		perror("Bind");

	/* Listen, accept, and go on */
	listen( ctrl_socket, 5 );
	printf("Listening on control port %d\n", control_port);
	char command[ cmd_len ];
	int ctrl_recv_size = recvfrom( ctrl_socket, &command, cmd_len, 0, (struct sockaddr *) &ctrl_address, &tmplen );
	close( ctrl_socket );


	// accepting incoming connections and measuring the time
	gettimeofday(&tv, NULL);
	accept_start = tv.tv_sec*1000000 + tv.tv_usec;
	for (k = 0; k < num_sender; k++) {
		connected[k] = accept(sock[k], (struct sockaddr *)&sender_addr[k], &sin_size);
		//printf("Received connection from %d\n", k);
	}
	gettimeofday(&tv, NULL);
	accept_end = tv.tv_sec*1000000 + tv.tv_usec;
	total_accept = accept_end - accept_start;

	// receiving data and measuring the time
	int partial_bytes;
	//double partial_time;
	gettimeofday(&tv, NULL);
	receive_start = tv.tv_sec*1000000 + tv.tv_usec;
	for (k = 0; k < packet_number; k++) {
		for (j = 0; j < num_sender; j++) {
			bytes_received = recv(connected[j], recv_data, message_size, MSG_WAITALL);
			total_bytes_recv += bytes_received;

			//printf("Partial received %d bytes\n", bytes_received);
		}
	}
	gettimeofday(&tv, NULL);
	receive_end = tv.tv_sec*1000000 + tv.tv_usec;
	total_time = receive_end - receive_start;

	// closing sockets
	for (k = 0; k < num_sender; k++)
		close(connected[k]);

	printf("Total accept() time: %f usec\n", total_accept);
	printf("Total bytes received %ld in %f usec\n", total_bytes_recv, total_time);
	printf("Bandwidth: %f Gb/s\n", (total_bytes_recv*8*1E6)/(1024*1024*1024*total_time));

	fflush(stdout);

	return 0;
}
