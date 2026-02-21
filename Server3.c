// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// Constants 
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 5
#define MALAYSIA_GMT_OFFSET 8
#define IP_ADDR_LEN 16  //Length for IPv4 address string (xxx.xxx.xxx.xxx\0) 

// Validates input to ensure ONLY contain letters, numbers, and spaces
int isAlphanumeric(const char *str) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (!isalnum((unsigned char)str[i]) && str[i] != ' ') {
            return 0;
        }
    }
    return 1; // All character are valid
}

// Get current time and adjust it to Malaysia timezone
void getMalaysiaTime(char *output) {
    time_t raw_time;  // Stores time as seconds since apoch
    struct tm *time_info;  // Stores broken-down time such as hour, min, and sec
    
    // Get current time 
    time(&raw_time);
    
    // Adjust to Malaysia time (GMT+8) 
    raw_time += (MALAYSIA_GMT_OFFSET * 3600);
    
    // Convert to UTC structure (offset already added) 
    time_info = gmtime(&raw_time);
    sprintf(output, "%02d:%02d:%02d\r\n",
            time_info->tm_hour,  // Hours
            time_info->tm_min,  // Minutes
            time_info->tm_sec);  // Seconds
}

// Gets full date and time information in a specific format.
void getCurrentDateTime(char *output) {
    time_t raw_time;
    struct tm *time_info;
    // Array to convert numeric values to readable string
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // Get current time 
    time(&raw_time);
    
    // Adjust to Malaysia time (GMT+8) 
    raw_time += (MALAYSIA_GMT_OFFSET * 3600);
    
    // Convert to UTC structure (offset already added) 
    time_info = gmtime(&raw_time);
    
    // Format output string 
    sprintf(output, "%s %s %02d %02d:%02d:%02d %d GMT+8\r\n",
            days[time_info->tm_wday],
            months[time_info->tm_mon],
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            time_info->tm_year + 1900);
}

// Check if the input string recognize "Date" and "Time"
int isValidCommand(const char *str) {
    if (strcmp(str, "Date") == 0 || strcmp(str, "Time") == 0) {
        return 1;  // Valid command
    }
    return 0;  // Not a valid command
}

// Handle all communication with ONE client
void handleClient(SOCKET client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];  // recieved message
    char response[BUFFER_SIZE];  // responses to send
    char client_ip[IP_ADDR_LEN];  // store client IP
    int client_port;  // Client's port number
    int bytes_received;  // number of bytes received
    
    // Extract client IP address and port 
    strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
    client_port = ntohs(client_addr.sin_port);
    
    // Display client connection information 
    printf("\n========================================\n");
    printf("New Connection Established:\n");
    printf("Client IP Address: %s\n", client_ip);
    printf("Client Port Number: %d\n", client_port);
    printf("========================================\n\n");
    
    // Handle multiple requests from the same client 
    while (1) {  // break when client disconnects
        // clear buffer to make sure that no leftover data
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        
        // Receive data from client 
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            // bytes_received == 0 means client closed connection properly
            // bytes_recieved < 0 means error occur
            printf("Client %s disconnected\n", client_ip);
            break; // Exit loop
        }
        
        buffer[bytes_received] = '\0';
        
        // Display received message information 
        printf("Message received from %s\n", client_ip);
        printf("Message length: %d bytes\n", bytes_received);
        printf("Message content: %s\n", buffer);
        
        // Check for exit command 
        if (strcmp(buffer, "Exit Server") == 0) {
            printf("Exit command received from %s. Closing connection...\n", client_ip);
            strcpy(response, "Server shutting down. Goodbye!");
            send(client_socket, response, strlen(response), 0);  // Close client's socket
            closesocket(client_socket);  
            return;
        }
        
        // Process "Date" command 
        if (strcmp(buffer, "Date") == 0) {
            getCurrentDateTime(response);  // Fill response with date/time
            send(client_socket, response, strlen(response), 0);  // Send to client
            printf("Date command processed. Response sent: %s", response);
            continue;
        }
        
        // Process "Time" command 
        if (strcmp(buffer, "Time") == 0) {
            getMalaysiaTime(response);
            send(client_socket, response, strlen(response), 0);
            printf("Time command processed. Malaysia time sent: %s", response);
            continue;
        }
        
        // Check for invalid characters 
        if (!isAlphanumeric(buffer)) {
            strcpy(response, "ERROR: Invalid input! Only alphanumeric characters or valid commands (e.g., 'Date', 'Time') are allowed.");
            send(client_socket, response, strlen(response), 0);
            printf("Error: Invalid characters detected. Error message sent to client.\n\n");
            continue;
        }
        
        // Check for invalid command 
        if (!isValidCommand(buffer)) {
            strcpy(response, "ERROR: Invalid command! Please use valid commands like 'Date' or 'Time' or send alphanumeric messages only.");
            send(client_socket, response, strlen(response), 0);
            printf("Error: Not a valid command. Error message sent to client.\n\n");
            continue;
        }
    }
    
    closesocket(client_socket);
}

// Set up server socket and accept the connection
int main(int argc, char *argv[]) {
    SOCKET server_fd, client_socket;
    struct sockaddr_in address, client_addr;
    int addrlen = sizeof(client_addr);
    int opt = 1;
    WSADATA wsaData;
    
    // Initialize Winsock 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Create socket 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();  // Clean up Winsock before exiting
        return 1;
    }
    
    // Set socket option so the I could reuse address 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        printf("Setsockopt failed\n");
    }
    
    // Configure server address structure 
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  //Accept on any network interface
    address.sin_port = htons(PORT);  // Convert port to network byte order
    
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
    printf("Supported commands: 'Date', 'Time', 'Exit Server'\n");
    printf("Waiting for client connections...\n");
    
    // Accept connections in a loop 
    while (1) {
        // Accept a new connection 
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        
        if (client_socket == INVALID_SOCKET) {
            printf("Accepting connection failed\n");
            continue;
        }
        
        // Handle the client 
        handleClient(client_socket, client_addr);
        
        // client sent "Exit Server" if ale to reach this step
        printf("Server terminating...\n");
        break;
    }
    
    // Close server socket and cleanup 
    closesocket(server_fd);
    WSACleanup();
    
    printf("Server terminated.\n");
    return 0;
}