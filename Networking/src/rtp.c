#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char *ip, char *port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/*
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
*/


/*
 *  Returns a number computed based on the data in the buffer.
*/
static int checksum(const char *buffer, int length){

	/*  ----  FIXME  ----
	 *
	 *  The goal is to return a number that is determined by the contents
	 *  of the buffer passed in as a parameter.  There a multitude of ways
	 *  to implement this function.  For simplicity, simply sum the ascii
	 *  values of all the characters in the buffer, and return the total.
	*/

	int sum = 0;

	for(int i = 0; i < length; i++) {
		sum += buffer[i];
	}

  return sum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it
 *  contains the length of the array created.
 */
static PACKET* packetize(const char *buffer, int length, int *count){

	/*  ----  FIXME  ----
	 *  The goal is to turn the buffer into an array of packets.
	 *  You should allocate the space for an array of packets and
	 *  return a pointer to the first element in that array.  Each
	 *  packet's type should be set to DATA except the last, as it
	 *  should be LAST_DATA type. The integer pointed to by 'count'
	 *  should be updated to indicate the number of packets in the
	 *  array.
	*/

	int position;
  int num_packets = (length + MAX_PAYLOAD_LENGTH - 1) / (MAX_PAYLOAD_LENGTH);
	*count = num_packets;
	PACKET* pack = calloc((size_t)num_packets, sizeof(PACKET));
	PACKET* packet;



	for(int i = 0; i < length; i++) {
		packet = pack + (i / MAX_PAYLOAD_LENGTH);
		position = (i % MAX_PAYLOAD_LENGTH);
		packet->payload[position] = buffer[i];

		if(i == (length - 1)) {
      packet->payload_length = position + 1;
      packet->checksum = checksum(packet->payload, packet->payload_length);
			packet->type = LAST_DATA;
    }
		if(position == MAX_PAYLOAD_LENGTH - 1) {
        packet->payload_length = MAX_PAYLOAD_LENGTH;
        packet->checksum = checksum(packet->payload, packet->payload_length);
				packet->type = DATA;
    }
	}

	return pack;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE *msg){
	/* ---- FIXME ----
	 * The goal of this function is to turn the message buffer
	 * into packets and then, using stop-n-wait RTP protocol,
	 * send the packets and re-send if the response is a NACK.
	 * If the response is an ACK, then you may send the next one
	*/

	int *counter = malloc(sizeof(int));

	 PACKET *pack = packetize(msg->buffer, msg->length, counter);
	 PACKET *response = malloc(sizeof(PACKET));

	 int socket = connection->socket;
	 socklen_t addrlen = connection->addrlen;
	 struct sockaddr *remote_addr = connection->remote_addr;

	 for (int i = 0; i < *counter; i++) {
		 sendto(socket, (void *)&pack[i], sizeof(PACKET), 0, remote_addr, addrlen);
		 recvfrom(socket, (void *)response, sizeof(PACKET), 0, remote_addr, &addrlen);

		 if (response->type == NACK) {
			 i--;
		 }

	 }

	 return 1;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	/* ---- FIXME ----
	 * The goal of this function is to handle
	 * receiving a message from the remote server using
	 * recvfrom and the connection info given. You must
	 * dynamically resize a buffer as you receive a packet
	 * and only add it to the message if the data is considered
	 * valid. The function should return the full message, so it
	 * must continue receiving packets and sending response
	 * ACK/NACK packets until a LAST_DATA packet is successfully
	 * received.
	*/

	int socket = connection->socket;
	socklen_t addrlen = connection->addrlen;
	struct sockaddr *remote_addr = connection->remote_addr;

	char *buffer = NULL;
	MESSAGE *message = malloc(sizeof(MESSAGE));
	PACKET *pack = malloc(sizeof(PACKET));

	size_t length = 0;
	int type = 0;
	do {
		recvfrom(socket, pack, sizeof(PACKET), 0, NULL, NULL);
		type = pack->type;
		if (checksum(pack->payload, pack->payload_length) == pack->checksum) {
			if (buffer == NULL) {
				buffer = (char*) malloc((size_t)pack->payload_length);
				strcpy(buffer, pack->payload);
				length = (size_t)pack->payload_length;
			} else {
			 	length += (size_t)pack->payload_length;
			 	buffer = (char*) realloc(buffer, length);
			 	strcat(buffer, pack->payload);
			}
			 	pack->type = ACK;
			 	sendto(socket, pack, sizeof(PACKET), 0, remote_addr, addrlen);
			} else {
			 	pack->type = NACK;
			 	sendto(socket, pack, sizeof(PACKET), 0, remote_addr, addrlen);
			}
	} while (type != LAST_DATA || (type == LAST_DATA && pack->type == NACK));

	message->buffer = buffer;
	message->length = length;

	return message;
}
