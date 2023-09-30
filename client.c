#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include "protocol.h"

// Define macros for debugging (set to 0 to disable debugging)
#define DEBUG 1

// Function to send a message to the server
int sendMessage(int socket, struct calcMessage *message, struct sockaddr_in serverAddr) {
    ssize_t sentBytes = sendto(socket, message, sizeof(struct calcMessage), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes == -1) {
        perror("sendto");
        return 0;
    }
    return 1;
}

// Function to receive a message from the server
int receiveMessage(int socket, struct calcProtocol *message, struct sockaddr_in *serverAddr) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    ssize_t receivedBytes = recvfrom(socket, message, sizeof(struct calcProtocol), 0, (struct sockaddr*)serverAddr, &addrSize);
    if (receivedBytes == -1) {
        perror("recvfrom");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip:port>\n", argv[0]);
        return 1;
    }

    // Parse server IP and port from command-line argument
    char* serverAddress = argv[1];

    // Create a UDP socket
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        perror("socket");
        return 1;
    }

    // Resolve the server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);

    struct hostent *serverInfo = gethostbyname(serverAddress);
    if (serverInfo == NULL) {
        fprintf(stderr, "Error resolving server address.\n");
        close(clientSocket);
        return 1;
    }
    memcpy(&serverAddr.sin_addr, serverInfo->h_addr, serverInfo->h_length);

    // Create and send the calcMessage to the server
    struct calcMessage initialMessage;
    initialMessage.type = htons(22);
    initialMessage.message = htonl(0);
    initialMessage.protocol = htons(17);
    initialMessage.major_version = htons(1);
    initialMessage.minor_version = htons(0);

    if (!sendMessage(clientSocket, &initialMessage, serverAddr)) {
        close(clientSocket);
        return 1;
    }

    // Receive and process the calcProtocol message from the server
    struct calcProtocol response;
    if (!receiveMessage(clientSocket, &response, &serverAddr)) {
        close(clientSocket);
        return 1;
    }

    // Process the response and print the assignment
    printf("Host %s, and port %d.\n", serverAddress, ntohs(serverAddr.sin_port));
    printf("ASSIGNMENT: %s %d %d\n", arithOpString(response.arith), ntohl(response.inValue1), ntohl(response.inValue2));

    // Simulate calculation (replace this with actual calculation)
    int result = 0;
    switch (ntohl(response.arith)) {
        case 1: // add
            result = ntohl(response.inValue1) + ntohl(response.inValue2);
            break;
        case 2: // sub
            result = ntohl(response.inValue1) - ntohl(response.inValue2);
            break;
        // Add more cases for other operations here
    }

    // Prepare and send the response
    response.type = htons(2);
    response.inResult = htonl(result);

    if (!sendMessage(clientSocket, &response, serverAddr)) {
        close(clientSocket);
        return 1;
    }

    // Receive the final response from the server
    if (!receiveMessage(clientSocket, &response, &serverAddr)) {
        close(clientSocket);
        return 1;
    }

    // Print the result
    printf("%s (myresult=%d)\n", response.message == htons(1) ? "OK" : "ERROR", ntohl(response.inResult));

    // Close the socket
    close(clientSocket);

    return 0;
}
