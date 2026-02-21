// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// Constants 
#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

// Wait for response before sending next message for client
int main(int argc, char *argv[]) {
    SOCKET sock;  // Socket for connection to server
    struct sockaddr_in serv_addr;  // Server address structure
    char buffer[BUFFER_SIZE];  // Receive data
    char message[BUFFER_SIZE];  // User input
    int bytes_received;  // Number of bytes recieved
    WSADATA wsaData;  // Winsock initialization data
    
    // Initialize Winsock 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Create socket 
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    
    // Configure server address structure 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Validates IP address
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    // Connect to the server 
    printf("Connecting to server...\n");
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Connection to server failed\n");
        printf("Make sure the server is running first!\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    printf("Connected to server successfully!\n");
    printf("You can now send messages to the server.\n");
    printf("Type 'Exit Server' to disconnect and terminate the server.\n");
    printf("========================================\n\n");
    
    // Main communication loop which able to send and receive multiple messages 
    while (1) {
        // Clear buffers 
        memset(message, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
        
        // Get input from user 
        printf("Enter message: ");
        fflush(stdout);  // Force output to display immediately
        
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline character 
        message[strcspn(message, "\n")] = '\0';
        
        // Check for empty input 
        if (strlen(message) == 0) {
            printf("Empty message. Please enter a valid message.\n\n");
            continue;  // Skip this iteration
        }
        
        // Send message to server 
        if (send(sock, message, strlen(message), 0) == SOCKET_ERROR) {
            printf("Failed to send message\n");
            break;
        }
        printf("Sent to server: %s\n", message);
        
        // Receive response from server 
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected\n");
            break;
        }
        
        buffer[bytes_received] = '\0';  // NULL-terminate recieved data
        printf("Received from server: %s\n\n", buffer);
        
        /* Check if we sent exit command */
        if (strcmp(message, "Exit Server") == 0) {
            printf("Closing connection...\n");
            break; // Exit loop
        }
    }
    
    // Close socket and cleanup 
    closesocket(sock);
    WSACleanup();
    
    printf("Client terminated.\n");
    return 0;
}