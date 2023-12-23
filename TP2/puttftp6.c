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
    int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
        perror("\n\nSocket error.\n\n");
        exit(EXIT_FAILURE);
    }

    printf("\n\nReservation | ai_family: %d\n    | ai_socktype: %d\n    | ai_protocol: %d\n\n",
           rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    return sfd;
}

// Function to send a packet to the server
int sendPacket(int sfd, char *packet, int size, struct sockaddr *peer_addr, socklen_t peer_addrlen) {
    int wasSent = sendto(sfd, packet, size, 0, peer_addr, peer_addrlen);
    if (wasSent == -1) {
        perror("Client: Error sending packet\n");
        exit(EXIT_FAILURE);
    }
    return wasSent;
}

// Function to receive a packet from the server
ssize_t receivePacket(int sfd, char *buffer, int buf_size, struct sockaddr *peer_addr, socklen_t *peer_addrlen) {
    ssize_t len = recvfrom(sfd, buffer, buf_size, 0, peer_addr, peer_addrlen);
    if (len == -1) {
        perror("Client: Error receiving packet\n");
        exit(EXIT_FAILURE);
    }
    return len;
}

// Function to handle file upload
void handleFileUpload(int sfd, struct addrinfo *rp, char *filename,char *blocksize) {
    char *WRQ;
    WRQ = malloc(strlen(filename) + 11);
    WRQ[0] = 0;
    WRQ[1] = 2; // Operation code for WRQ
    strcpy(WRQ + 2, filename);
    WRQ[2 + strlen(filename)] = 0;
    strcpy(WRQ + 3 + strlen(filename), "octet");
    WRQ[3 + strlen(filename) + strlen("octet")] = 0;
    sprintf(WRQ + 4 + strlen(filename) + strlen("octet"), "blksize%c%s", 0, blocksize);

    int size = 3 + strlen(filename) + strlen("octet") + strlen(blocksize) + 1;

    sendPacket(sfd, WRQ, size, rp->ai_addr, rp->ai_addrlen);

    // Receive acknowledgment (ACK) from the server
    char buffer[BUF_SIZE];
    receivePacket(sfd, buffer, BUF_SIZE, rp->ai_addr, &rp->ai_addrlen);
    if (buffer[1] == 4) { // ACK received
        printf("ACK received.\n");
    }

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

        size = sendPacket(sfd, packet, size + 4, rp->ai_addr, rp->ai_addrlen);

        if (size < BUF_SIZE) {
            eof = 1;
        }

        // Receive acknowledgment (ACK) from the server
        receivePacket(sfd, buffer, size, rp->ai_addr, &rp->ai_addrlen);
        if (buffer[1] == 4) { // ACK received
            printf("ACK received.\n");
        }
        blocknumber++;
        free(packet);
    }
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
    if (argc != 5) {
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

    // Check if the program is "puttftp" for file upload

        handleFileUpload(sfd, rp, argv[3],argv[4]);

    freeaddrinfo(rp); // Free the address info
    close(sfd);       // Close the socket
    return 0;
}
