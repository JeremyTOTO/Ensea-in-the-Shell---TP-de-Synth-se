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
        if (errorCode == 1) perror("Client: Error sending the WRQ request.\n");
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

// Function to handle file upload
void handleFileUpload(int sfd, struct addrinfo *rp, char *filename) {
	
	char buffer[BUF_SIZE];
    char *WRQ;
    
    WRQ = malloc(strlen(filename) + 9);
    WRQ[0] = 0;
    WRQ[1] = 2; // Operation code for WRQ
    strcpy(WRQ + 2, filename);
    WRQ[2 + strlen(filename)] = 0;
    strcpy(WRQ + 3 + strlen(filename), "octet");
    WRQ[3 + strlen(filename) + strlen("octet")] = 0;

    int size = 3 + strlen(filename) + strlen("octet") + 1;

    size = sendPacket(sfd, WRQ, size, rp->ai_addr, rp->ai_addrlen,1);
    
	int sizeOfMessage = sprintf(buffer, " CLIENT: The %d bytes WRQ request has been sended successfully.\n\n",size);
	write(1,buffer,sizeOfMessage); //Display RRQ request status in the terminal. 

    // Receive acknowledgment (ACK) from the server
    receivePacket(sfd, buffer, BUF_SIZE, rp->ai_addr, &rp->ai_addrlen,4);
    if (buffer[1] == 4) { // ACK received
    }
    	sizeOfMessage = sprintf(buffer, " CLIENT: ACK autorizing file upload to the server -> [%d%d][%d%d]\n\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		write(1,buffer,sizeOfMessage); //Display the first ACK sended in the terminal.

    // Send a file consisting of a single Data (DAT) packet
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Client: Error opening file\n");
        exit(EXIT_FAILURE);
    }

    u_int16_t blocknumber = 1;
    int eof = 0;
    while (!eof) {
        char data[516]; // First data packet
        size = fread(data, 1, BUF_SIZE, file);

        // Construct a packet to be sent
        char *packet;
        packet = malloc(size + 4);
        packet[0] = 0;
        packet[1] = 3;
        packet[2] = 0;
        packet[3] = blocknumber;
        memcpy(packet + 4, data, size);

        size = sendPacket(sfd, packet, size + 4, rp->ai_addr, rp->ai_addrlen,3);
        
        sizeOfMessage = sprintf(buffer, " CLIENT: %d bytes of DATA have been sended to the server.\n", size-4);
		write(1,buffer,sizeOfMessage); //Display the amount of data sended in the terminal.

        if (size < BUF_SIZE) {
            eof = 1;
        }

        // Receive acknowledgment (ACK) from the server
        receivePacket(sfd, buffer, size, rp->ai_addr, &rp->ai_addrlen,4);
        if (buffer[1] == 4) { // ACK received
        }
        
        sizeOfMessage = sprintf(buffer, " CLIENT: ACK of data sended to the server -> [%d%d][%d%d]\n\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		write(1,buffer,sizeOfMessage); //Display the other ACK sended in the terminal.
		
        blocknumber++;
        free(packet);
    }
    
	sizeOfMessage = sprintf(buffer, " The downloading from the server is complete.\n\n");
	write(1,buffer,sizeOfMessage); //Display the downloading state in the terminal. 
		
    fclose(file);
    free(WRQ);
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

    // Handle the file upload

        handleFileUpload(sfd, rp, argv[3]);

    freeaddrinfo(rp); // Free the address info
    close(sfd);       // Close the socket
    return 0;
}
