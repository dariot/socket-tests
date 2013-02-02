#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{

	if(argc < 5) {
		printf("\nUsage: ./sender1N <num_receiver> <receiver_ip_last_byte> <receiver_port> <packet_size_in_bytes> <packet_number> <control_port>\n\n");
		exit(1);
	}

	int num_socket = atoi(argv[1]);
	int packet_size = atoi(argv[1 + num_socket + 2]);
	int message_size = packet_size, bytes_received, true = 1;
	char send_data[message_size], recv_data[message_size];
	double send_end, send_start;

	int connected[num_socket];
	struct hostent *host[num_socket];
	struct sockaddr_in receiver[num_socket], this_addr[num_socket];
	int port = atoi(argv[1 + num_socket + 1]);
	printf("Port: %d\n", port);
	int sock[num_socket], recv_sock[num_socket];

	int packet_number = atoi(argv[1 + num_socket + 3]);
	int j;

	struct timeval tv;

	int i;
	for (i = 0; i < message_size; i++)
		send_data[i] = 'a';

	int k = 0;
	/*
	   while (k < num_socket) {
	   char str[20];
	   strcpy(str, "192.168.2.");
	   strcat(str, argv[k+4]);

	   host[k] = gethostbyname(str);
	   port[k] = atoi(argv[2]);
	   k++;
	   }
	   */

	int sin_size = sizeof(struct sockaddr_in);
	char receiver_ip[num_socket][20];

	for (k = 0; k < num_socket; k++) {

		strcpy(receiver_ip[k], "192.168.2.");
		strcat(receiver_ip[k], argv[2+k]);
		host[k] = gethostbyname(receiver_ip[k]);
		printf("receiver[%d]: %s\n", k, receiver_ip[k]);

		if ((sock[k] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
		}

		receiver[k].sin_family = AF_INET;
		receiver[k].sin_port = htons(port);
		receiver[k].sin_addr = *((struct in_addr *)host[k]->h_addr);
		bzero(&(receiver[k].sin_zero), 8);

		if(setsockopt(sock[k], SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
			perror("Setsockopt");
			exit(1);
		}


		/* create receiving socket for ACK reception */
		if ((recv_sock[k] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
		}

		if (setsockopt(recv_sock[k], SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
			perror("Setsockopt SO_REUSEADDR");
			exit(1);
		}

		this_addr[k].sin_family = AF_INET;
		this_addr[k].sin_port = htons(port + k);
		this_addr[k].sin_addr.s_addr = INADDR_ANY;
		bzero(&(this_addr[k].sin_zero), 8);

		if (bind(recv_sock[k], (struct sockaddr *)&this_addr[k], sizeof(struct sockaddr)) == -1) {
			perror("Unable to bind");
			exit(1);
		}

		if (listen(recv_sock[k], 1) == -1) {
			perror("Listen");
			exit(1);
		}

		printf("Listening for ACK from %s on port %d\n", receiver_ip[k], port + k);
	}


	int control_port = atoi(argv[1 + num_socket + 4]); /* DEFAULT control UDP port to receive the
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
		perror("Control bind");

	/* Listen, accept, and go on */
	listen( ctrl_socket, 5 );
	printf("Listening on control port %d\n", control_port);
	char command[ cmd_len ];
	int ctrl_recv_size = recvfrom( ctrl_socket, &command, cmd_len, 0, (struct sockaddr *) &ctrl_address, &tmplen );
	close( ctrl_socket );

	double conn_start, conn_end, conn_time;
	gettimeofday(&tv, NULL);
	conn_start = tv.tv_sec * 1E6 + tv.tv_usec;
	for (k = 0; k < num_socket; k++) 
		if(connect(sock[k], (struct sockaddr *)&receiver[k], sizeof(struct sockaddr)) == -1) {
			perror("Connect");
			exit(1);
		}
	gettimeofday(&tv, NULL);
	conn_end = tv.tv_sec * 1E6 + tv.tv_usec;
	conn_time = conn_end - conn_start;

	long total_bytes = 0, bytes_sent = 0;
	gettimeofday(&tv, NULL);
	send_start = tv.tv_sec * 1E6 + tv.tv_usec;
	for (j = 0; j < packet_number; j++)
		for (k = 0; k < num_socket; k++) {
			bytes_sent = send(sock[k], send_data, message_size, 0);
			total_bytes += bytes_sent;
			//printf("Sending %ld bytes to %d\n", bytes_sent, k);
		}
	gettimeofday(&tv, NULL);
	send_end = tv.tv_sec * 1E6 + tv.tv_usec;
	double send_time = send_end - send_start;

	double ack_start[num_socket], ack_end[num_socket], ack_time[num_socket];
	double total_ack_start, total_ack_end, total_ack_time;

	gettimeofday(&tv, NULL);
	total_ack_start = tv.tv_sec * 1E6 + tv.tv_usec;
	for (k = 0; k < num_socket; k++) {
		gettimeofday(&tv, NULL);
		ack_start[k] = tv.tv_sec * 1E6 + tv.tv_usec;

		connected[k] = accept(recv_sock[k], (struct sockaddr *)&receiver[k], &sin_size);
		while (recv(connected[k], recv_data, 3, 0) == 0) {}

		gettimeofday(&tv, NULL);
		ack_end[k] = tv.tv_sec * 1E6 + tv.tv_usec;
		ack_time[k] = ack_end[k] - ack_start[k];

		printf("Received ACK from %s in %.0f usec\n", receiver_ip[k], ack_time[k]);
	}
	gettimeofday(&tv, NULL);
	total_ack_end = tv.tv_sec * 1E6 + tv.tv_usec;
	total_ack_time = total_ack_end - total_ack_start;


	for (k = 0; k < num_socket; k++) {
		close(sock[k]);
		close(connected[k]);
	}

	double bandwidth = (total_bytes * 8 * 1E6) / ((total_ack_time + send_time) * ( 1 << 30 ) );
	printf("Connect() time: %.0f usec\n", conn_time);
	printf("Send() time: %.0f usec\n", send_time);
	printf("Total ACK time: %.0f usec\n", total_ack_time);
	printf("Sent %ld bytes in %.0f usec\n", total_bytes, total_ack_time);
	printf("Bandwidth sender: %.2f Gb/s\n", bandwidth);
	printf("\n");

	return 0;
}
