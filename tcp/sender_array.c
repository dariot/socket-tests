/*********************
 * TCP SOCKET TEST 3
 * ******************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{

	if(argc < 5) {
		printf("\nUsage: ./sender_array <num_socket> <receiver_ip_last_byte> <receiver_first_port> <packet_size_in_KB> <packet_number> <control_port>\n\n");
		exit(1);
	}

	int packet_size = atoi(argv[4]);
	int message_size = packet_size*1024, bytes_received, connected, true = 1;
	char send_data[message_size];
	double end, start;

	int num_socket = atoi(argv[1]);
	struct hostent *host;
	struct sockaddr_in receiver[num_socket];
	int port = atoi(argv[3]);
	int sock[num_socket];

	int packet_number = atoi(argv[5]);
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
	char receiver_ip[20];
	strcpy(receiver_ip, "192.168.2.");
	strcat(receiver_ip, argv[2]);
	host = gethostbyname(receiver_ip);

	int sin_size = sizeof(struct sockaddr_in);

	for (k = 0; k < num_socket; k++) {
		if ((sock[k] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
		}

		receiver[k].sin_family = AF_INET;
		receiver[k].sin_port = htons(port + k);
		receiver[k].sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(receiver[k].sin_zero), 8);

		if(setsockopt(sock[k], SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
			perror("Setsockopt");
			exit(1);
		}
	}


	int control_port = atoi(argv[6]); /* DEFAULT control UDP port to receive the
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


	for (k = 0; k < num_socket; k++) 
		if(connect(sock[k], (struct sockaddr *)&receiver[k], sizeof(struct sockaddr)) == -1) {
			perror("Connect");
			exit(1);
		}

	long total_bytes = 0, bytes_sent = 0;
	gettimeofday(&tv, NULL);
	start = tv.tv_sec + tv.tv_usec/1000000.0;
	for (j = 0; j < packet_number; j++)
		for (k = 0; k < num_socket; k++) {
			bytes_sent = send(sock[k], send_data, message_size, 0);
			total_bytes += bytes_sent;
			//printf("Sending %ld bytes to %d\n", bytes_sent, k);
		}
	gettimeofday(&tv, NULL);
	end = tv.tv_sec + tv.tv_usec/1000000.0;
	double time = end - start;

	for (k = 0; k < num_socket; k++)
		close(sock[k]);

	printf("Sent %ld bytes in %f usec\n", total_bytes, time*1000000);

	return 0;
}
