/*********************
 * TCP SOCKET TEST 3
 * ******************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tprof.h"

#define NET_BAND 1E10 // network bandwidth in bits/sec

unsigned long foo = 0;

int n_prof_sections = 10;

int main( int argc, char *argv[] ) {

  if(argc < 3) {
    printf("\nUsage: ./sender <receiver_address> <receiver_port> <packet_size_in_bytes> <packet_number> <control_port> <sleep_factor>\n\n");
    exit(1);
  }

  /* init profiling */
  prinit( n_prof_sections );

  /* estimated waste cycles per seconds */
  double cps = cycles_per_sec();

  double sleep_factor = 0;
  
  //int control_port = 10000;
  int packet_size = atoi(argv[3]);
  int sock, control, bytes_received, connectedi, true = 1;
  long message_size = packet_size;
  char send_data[message_size], recv_data[message_size];
  
  struct hostent *host;
  struct sockaddr_in receiver_addr, this_addr;
  host = gethostbyname(argv[1]);
  int port = atoi(argv[2]);
  int packet_number = atoi(argv[4]);

  if ( argc > 6 )
    sleep_factor = atof( argv[ 6 ] );

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
	/*
	for (i = 0; i < message_size; i++)
		send_data[i] = 'a';
	*/
	memset(send_data, 1, packet_size*(sizeof(char)/8));

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

	// connecting the socket and measuring the time
	gettimeofday(&tv, NULL);
	connect_start = tv.tv_sec + (tv.tv_usec/1000000.0);
	if(connect(sock, (struct sockaddr *)&receiver_addr, sin_size) == -1)
	{
		perror("Connect");
		exit(1);
	}
	gettimeofday(&tv, NULL);
	connect_end = tv.tv_sec + (tv.tv_usec/1000000.0);
	total_connect = connect_end - connect_start;

	// sending data and measuring the time
	double send_start, send_end, send_time;
	long total_bytes_sent = 0, bytes_sent = 0;
	gettimeofday(&tv, NULL);
	send_start = tv.tv_sec*1000000 + tv.tv_usec;

	double mesg_send_time = sleep_factor * packet_size * 8 / NET_BAND;
	double sleep_time = 1 * mesg_send_time;

	printf( "theoretical sleep: %f sec\n", sleep_time );

	for (i = 0; i < packet_number; i++) {

	  bytes_sent = send( sock, send_data, message_size, 0 );
	  total_bytes_sent += bytes_sent;
		
	  //	  prstart( 1 );
	  
	  foo += waste_cpu( cps * sleep_time );
	  
	  //	  prstop( 1 );
	  
	  //	  double t; prteval( 1, &t );
	  //	  printf( "[DEBUG] measured sleep: %f s\n", t );
	  
	  //fflush(sock);
	}
	gettimeofday(&tv, NULL);
	send_end = tv.tv_sec*1000000 + tv.tv_usec;
	send_time = send_end - send_start;

	// closing socket and printing info
	close(sock);

	printf("Connect() time: %f usec\n", total_connect*1000000);
	printf("Send() time: %f usec\n", send_time);
	printf("Sent %ld bytes\n", total_bytes_sent);
	printf("Bandwidth: %f Gb/s\n", (total_bytes_sent*8*1E6)/(1024*1024*1024*send_time));

	printf("FOO %d\n", foo );
	printf("\n");

	fflush(stdout);

	return 0;
}

