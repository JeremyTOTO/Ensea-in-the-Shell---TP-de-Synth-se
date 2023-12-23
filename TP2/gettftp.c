#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 512

// Function to create a socket and print address info reservation details
int createSocket(struct addrinfo *rp) {
	char textBuffer[BUF_SIZE];
    int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
        perror("\n\nSocket error.\n\n");
        exit(EXIT_FAILURE);
    } else {
		int sizeOfMessage = sprintf(textBuffer, "\n\n CLIENT: The socket has been reserved successfully.\n");
		write(1,textBuffer,sizeOfMessage); //Display the successful reservation of the socket in the terminal. 		
		}
    return sfd;
}

// Function to send a packet to the server
int sendPacket(int sfd, char *packet, int size, struct sockaddr *peer_addr, socklen_t peer_addrlen, int errorCode) {
    int wasSent = sendto(sfd, packet, size, 0, peer_addr, peer_addrlen);
    if (wasSent == -1) {
        if (errorCode == 1) perror("Client: Error sending the RRQ request.\n");
        if (errorCode == 2) perror("Client: Error sending the ACK.\n");
        exit(EXIT_FAILURE);
    }
    return wasSent;
}

// Function to receive a packet from the server
ssize_t receivePacket(int sfd, char *buffer, int buf_size, struct sockaddr *peer_addr, socklen_t *peer_addrlen, int errorCode) {
    ssize_t len = recvfrom(sfd, buffer, buf_size, 0, peer_addr, peer_addrlen);
    if (len == -1) {
        if (errorCode == 3) perror("Client: Error receiving DATA from the server.\n");
        if (errorCode == 4) perror("Client: Error receiving ACK from the server.\n");
        exit(EXIT_FAILURE);
    }
    return len;
}

// Function to handle file download
void handleFileDownload(int sfd, struct addrinfo *rp, char *filename) {
	
	int block = 1;
    ssize_t len;
    char buffer[BUF_SIZE];
	
    // Construct a Read Request (RRQ) and send it to the server
    char *RRQ;
    RRQ = malloc(strlen(filename) + 9);
    RRQ[0] = 0;
    RRQ[1] = 1;
    strcpy(RRQ + 2, filename);
    RRQ[2 + strlen(filename)] = 0;
    strcpy(RRQ + 3 + strlen(filename), "octet");
    RRQ[3 + strlen(filename) + strlen("octet")] = 0;


    int size;
    size = 3 + strlen(filename) + strlen("octet") + 1;

    size = sendPacket(sfd, RRQ, size, rp->ai_addr, rp->ai_addrlen,1);

	int sizeOfMessage = sprintf(buffer, " CLIENT: The %d bytes RRQ request has been sended successfully.\n\n",size);
	write(1,buffer,sizeOfMessage); //Display RRQ request status in the terminal. 
		
    // Receive a file from the server

    do {
        // Receive data packet from the server
        len = receivePacket(sfd, buffer, BUF_SIZE, rp->ai_addr, &rp->ai_addrlen,3);
        
        int sizeOfMessage = sprintf(buffer, " CLIENT: %ld bytes of DATA have been received from the server.\n", len);
		write(1,buffer,sizeOfMessage); //Display the amount of data received in the terminal.
		
        if (len == -1) {
            perror("Client : Erreur 'recevfrom' \n");
            exit(EXIT_FAILURE);
        }

        buffer[len] = '\0';

        // Write data to the file
        FILE *fichiertelecharge = fopen(filename, "ab");
        fwrite(buffer +4, sizeof(char), len - 4, fichiertelecharge);
        fclose(fichiertelecharge);


        // Send acknowledgment (ACK) to the server
     
        char ack[4] = {0, 4, block >> 8, block & 0xff}; // ACK for the current block

        sendPacket(sfd, ack, 4, rp->ai_addr, rp->ai_addrlen,2);

		sizeOfMessage = sprintf(buffer, " CLIENT: ACK of data received from the server -> [%d%d][%d%d]\n\n", ack[0], ack[1], ack[2], ack[3]);
		write(1,buffer,sizeOfMessage); //Display the ACK sended in the terminal. 

        //printf("\n\n%li\n\n", len);
        block++;
    } while (len == BUF_SIZE); // While the data packet is of maximum size
    
		sizeOfMessage = sprintf(buffer, " The downloading from the server is complete.\n\n");
		write(1,buffer,sizeOfMessage); //Display the downloading state in the terminal. 
    
    free(RRQ);
    close(sfd);
}

int main(int argc, char *argv[]) {
    struct addrinfo hints, *rp;

    // Set up address info hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = IPPROTO_UDP; /* Any protocol */

    // Check if command line arguments are valid
    if (argc != 4) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get address info for the specified port
    int s = getaddrinfo(argv[1], argv[2], &hints, &rp);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Create socket
    int sfd = createSocket(rp);

    // Handle the file download

        handleFileDownload(sfd, rp, argv[3]);

    freeaddrinfo(rp); // Free the address info
    close(sfd);       // Close the socket
    return 0;
}
