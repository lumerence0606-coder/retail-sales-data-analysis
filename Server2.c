// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") //Link Winsock library

// Constants 
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 5 //Maximum pending connection in queue
#define IP_ADDR_LEN 16  //Length for IPv4 address string (xxx.xxx.xxx.xxx\0) 

// Function Prototypes 
int isAlphanumeric(const char *str);
void reverseAndUppercase(const char *input, char *output);
void handleClient(SOCKET client_socket, struct sockaddr_in client_addr);

// Validate if string contains only alphanumeric characters and spaces
int isAlphanumeric(const char *str) {
    int i;
    
    if (str == NULL) { // Safety check for null pointer
        return 0;
    }
    
    for (i = 0; str[i] != '\0'; i++) {
        if (!isalnum((unsigned char)str[i]) && str[i] != ' ') {
            return 0; // Invalid character found
        }
    }
    return 1; // All character valid
}

//Revert string and converts letters to uppercase
void reverseAndUppercase(const char *input, char *output) {
    int len;
    int i;
    int j;
    
    if (input == NULL || output == NULL) { 
        return;
    }
    
    len = strlen(input); //Get length of input
    j = 0; // Index for output string
    
    // Reverse the string and convert to uppercase 
    for (i = len - 1; i >= 0; i--) {
        if (isalpha((unsigned char)input[i])) { // If character is a letter
            output[j] = toupper((unsigned char)input[i]); // Convert to uppercase
        } else {
            output[j] = input[i]; // Keep non-letters as-is
        }
        j++;
    }
    output[j] = '\0'; //Add null terminator
}

//Handles all communication with a single client
void handleClient(SOCKET client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char client_ip[IP_ADDR_LEN];  /* Fixed: Using defined constant instead of INET_ADDRSTRLEN */
    int client_port;
    int bytes_received;
    char *ip_str;
    
    // Extract client IP address and port 
    ip_str = inet_ntoa(client_addr.sin_addr); // Convert network address to string
    if (ip_str != NULL) {
        strncpy(client_ip, ip_str, IP_ADDR_LEN - 1); // Copy IP
        client_ip[IP_ADDR_LEN - 1] = '\0';  // Ensure null termination 
    } else {
        strcpy(client_ip, "Unknown");
    }
    
    client_port = ntohs(client_addr.sin_port);  // Convert port from network to host byte order
    
    // Display client connection information 
    printf("\n========================================\n");
    printf("New Connection Established:\n");
    printf("Client IP Address: %s\n", client_ip);
    printf("Client Port Number: %d\n", client_port);
    printf("========================================\n\n");
    
    // Handle multiple requests from the same client 
    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear buffers
        memset(response, 0, BUFFER_SIZE);
        
        // Receive data from client 
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            printf("Client %s disconnected\n", client_ip); // Connection close or error
            break;
        }
        
        // Null-terminate received data 
        buffer[bytes_received] = '\0';
        
        // Display received message information 
        printf("Message received from %s\n", client_ip);
        printf("Message length: %d bytes\n", bytes_received);
        printf("Message content: %s\n", buffer);
        
        // Check for exit command 
        if (strcmp(buffer, "Exit Server") == 0) {
            printf("Exit command received from %s. Closing connection...\n", client_ip);
            strcpy(response, "Server shutting down. Goodbye!");
            send(client_socket, response, strlen(response), 0);
            closesocket(client_socket);
            return;
        }
        
        // Validate alphanumeric input 
        if (!isAlphanumeric(buffer)) {
            strcpy(response, "ERROR: Only alphanumeric characters are allowed! Please send alphanumeric format message only.");
            send(client_socket, response, strlen(response), 0);
            printf("Error: Non-alphanumeric characters detected. Error message sent to client.\n\n");
            continue;
        }
        
        // Process valid input: reverse and convert to uppercase 
        reverseAndUppercase(buffer, response);
        
        // Send response to client 
        send(client_socket, response, strlen(response), 0);
        printf("Response sent to client: %s\n\n", response);
    }
    
    closesocket(client_socket); // Close client socket when done
}

// Main server function
int main(int argc, char *argv[]) {
    SOCKET server_fd; // Server socket 
    SOCKET client_socket; // Client socket
    struct sockaddr_in address;  // Server address
    struct sockaddr_in client_addr;  // Client address
    int addrlen;
    int opt;
    WSADATA wsaData;  // Winsock data structure
    
    // Suppress unused parameter warnings 
    (void)argc;
    (void)argv;
    
    addrlen = sizeof(client_addr);
    opt = 1;
    
    // Initialize Winsock 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error: WSAStartup failed\n");
        return 1;
    }
    
    // Create socket 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    
    // Set socket option to reuse address (help avoid "address already in use" errors)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        printf("Setsockopt failed\n");
    }
    
    // Configure server address structure 
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); //Port in network byte order
    
    // Bind socket to IP and port 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Binding failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    // Listen for incoming connections 
    if (listen(server_fd, MAX_CONNECTIONS) == SOCKET_ERROR) {
        printf("Listening failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    printf("Server is listening on port %d...\n", PORT);
    printf("Waiting for client connections...\n");
    
    // Accept connections in a loop 
    while (1) {
        //Accept a new connection
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen); 
        
        if (client_socket == INVALID_SOCKET) {
            printf("Accepting connection failed\n");
            continue;  // Try accepting next connection
        }
        
        // Handle the client (multiple requests from same session) 
        handleClient(client_socket, client_addr);
        
        // If we reach here, client sent "Exit Server" 
        printf("Server terminating...\n");
        break;
    }
    
    // Close server socket and cleanup 
    closesocket(server_fd);
    WSACleanup();
    
    printf("Server terminated.\n");
    return 0;
}