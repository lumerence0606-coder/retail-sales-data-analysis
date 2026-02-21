// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>  
#include <ws2tcpip.h>   

#pragma comment(lib, "Ws2_32.lib")  

#define close(fd) closesocket(fd) 

// Automatic Winsock initialization and cleanup
static void init_winsock() __attribute__((constructor));
static void cleanup_winsock() __attribute__((destructor));

static void init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }
}

static void cleanup_winsock() {
    WSACleanup();
}

#define PORT 8080  // Port number for the server

// Check if string ONLY contain letters, numbers, and spaces
int isAlphanumeric(const char *str) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i]) && str[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

// Convert all letters to uppercase
void convertToUppercase(char *str) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

// Reverse a string in place
void reverseString(char *str) {
    int length = strlen(str); // Get string length
    int i;
    char temp;
    
    for (i = 0; i < length / 2; i++) { //Loop through first half of string
        temp = str[i]; //Store first character temporarily
        str[i] = str[length - 1 - i]; //Replace first with responding last
        str[length - 1 - i] = temp;   //Replace last with stored first
    }
}

//Main processing function that validates, uppercases, and reverses input
int processMessage(const char *input, char *output) {
    // Check if input have invalid characters
    if (!isAlphanumeric(input)) {
        strcpy(output, "Error: Only alphanumeric characters are allowed");
        return 0; // Return failure
    }
    
    // Copy input to output
    strcpy(output, input);
    
    // Convert to uppercase
    convertToUppercase(output);
    
    // Reverse the string
    reverseString(output);
    
    return 1; // Return success
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address; //Structure to hold server address info
    int addrlen = sizeof(address);
    char buffer[1024] = {0};  //Buffer to recieve data from client
    char response[1024] = {0}; //Buffer to send response to client
    int running = 1;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {  // Check if socket creation failed
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to an IP/Port
    address.sin_family = AF_INET;          // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connection on any network interface
    address.sin_port = htons(PORT);        // Convert port to network byte order
    
    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        close(server_fd); // Close socket before exit
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);

    // Accept a connection
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket == -1) {
        perror("Accepting connection failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected successfully!\n");

    // Message loop which keep receiving and responding until "Exit Server"
    while (running) {
        // Clear buffers
        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));
        
        // Receive data from the client
        int bytes_received = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }
        
        // Null-terminate the received data
        buffer[bytes_received] = '\0';
        
        // Remove trailing newline or carriage return character
        int len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[len - 1] = '\0';
            if (len > 1 && (buffer[len - 2] == '\r' || buffer[len - 2] == '\n')) {
                buffer[len - 2] = '\0';
            }
        }
        
        printf("Received from client: %s\n", buffer);
        
        // Check if client wants to exit
        if (strcmp(buffer, "Exit Server") == 0) {
            strcpy(response, "Server closing connection. Goodbye!");
            send(new_socket, response, strlen(response), 0);
            printf("Exit command received. Closing connection...\n");
            running = 0; // Exit loop
            break;
        }
        
        // Process the message
        processMessage(buffer, response);
        
        // Send response back to client
        send(new_socket, response, strlen(response), 0);
        printf("Sent to client: %s\n", response);
    }

    // Cleanup and close the sockets
    close(new_socket);
    close(server_fd);
    
    printf("Server terminated.\n");

    return 0;
}