/*********************
 * TCP SOCKET TEST 3
 * ******************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{

	if(argc < 3) {
		printf("\nUsage: ./sender <receiver_address> <receiver_port> <packet_size_in_bytes> <packet_number> <control_port> <num_sender>\n\n");
		exit(1);
	}

	int packet_size = atoi(argv[3]);
	int num_sender = atoi(argv[6]);
	int sock, recv_sock, control, bytes_received, connected, true = 1;
	long message_size = packet_size;
	char send_data[message_size], recv_data[message_size];

	struct hostent *host;
	struct sockaddr_in receiver_addr, this_addr;
	host = gethostbyname(argv[1]);
	int port = atoi(argv[2]);
	int packet_number = atoi(argv[4]);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
		perror("Setsockopt");
		exit(1);
	}

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &true, sizeof(int)) == -1)
	{
		perror("Setsockopt TCP_NODELAY");
		exit(1);
	}

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(port);
	receiver_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(receiver_addr.sin_zero),8);

	int sin_size = sizeof(struct sockaddr_in);

	int i;
	for (i = 0; i < message_size; i++)
		send_data[i] = 'a';

	/*
	   if(connect(sock, (struct sockaddr *)&receiver_addr, sin_size) == -1)
	   {
	   perror("Connect");
	   exit(1);
	   }
	   */


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

	struct timeval tv;
	double connect_start, connect_end, total_connect;

	gettimeofday(&tv, NULL);
	connect_start = tv.tv_sec * 1E6 + tv.tv_usec;
	if(connect(sock, (struct sockaddr *)&receiver_addr, sin_size) == -1)
	{
		perror("Connect");
		exit(1);
	}
	gettimeofday(&tv, NULL);
	connect_end = tv.tv_sec * 1E6 + tv.tv_usec;
	total_connect = connect_end - connect_start;

	double send_start = 0, send_end, send_time = 0;
	long total_bytes_sent = 0, bytes_sent = 0;
	gettimeofday(&tv, NULL);
	send_start = tv.tv_sec * 1E6 + tv.tv_usec;
	for (i = 0; i < packet_number; i++) {
		bytes_sent = send(sock, send_data, message_size, 0);
		//printf("Partial bytes sent: %ld\n", bytes_sent);
		total_bytes_sent += bytes_sent;
		//fflush(sock);
	}
	gettimeofday(&tv, NULL);
	send_end = tv.tv_sec * 1E6 + tv.tv_usec;
	send_time = send_end - send_start;
	
	/* Create the socket and listen for the receiver's ACK */
	if ((recv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}

	/*
	if (setsockopt(recv_sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
		perror("Setsockopt SO_REUSEADDR");
		exit(1);
	}
	*/

	this_addr.sin_family = AF_INET;
	this_addr.sin_port = htons(control_port);
	this_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(this_addr.sin_zero), 8);

	if (bind(recv_sock, (struct sockaddr *)&this_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
	}

	if (listen(recv_sock, 1) == -1) {
		perror("Listen");
		exit(1);
	}

	//printf("Listening on port %d\n", control_port);

	double ack_start, ack_end, ack_time;
	gettimeofday(&tv, NULL);
	ack_start = tv.tv_sec * 1E6 + tv.tv_usec;
	
	connected = accept(recv_sock, (struct sockaddr*)&receiver_addr, &sin_size);
	while ( recv(connected, recv_data, 3, 0) == 0) {}
	//printf("ACK received\n");
	
	gettimeofday(&tv, NULL);
	ack_end = tv.tv_sec * 1E6 + tv.tv_usec;
	ack_time = ack_end - ack_start;

	/* close all sockets */
	close(sock);
	close(connected);

	double band = (total_bytes_sent * 8 * 1E6) / ((send_time + ack_time) * pow(2, 30));
	double norm_band = (10 / num_sender) / band;

	printf("Connect() time: %.0f usec\n", total_connect);
	printf("Send() time: %.0f usec\n", send_time);
	printf("ACK receive time: %.0f usec\n", ack_time);
	printf("Total bytes sent: %ld\n", total_bytes_sent);
	printf("Bandwidth Sender: %.2f Gb/s\n", band);
	printf("Bandwidth normalized sender: %.2f Gb/s\n", norm_band);
	printf("\n");

	fflush(stdout);

	return 0;
}

