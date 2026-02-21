// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>  // For window threading

#pragma comment(lib, "ws2_32.lib")

// Constants
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 2  // Maximum number of client 
#define IP_ADDR_LEN 16  // Length for IPv4 address string (xxx.xxx.xxx.xxx\0)

// Structure hold all information about ONE connected client
typedef struct {
    SOCKET socket;  // Socket for specific client
    char ip[IP_ADDR_LEN];  // Client IP address
    int port;  // Client port number
    int id;  // Unique ID assigned
} ClientInfo;

// Global variables 
static ClientInfo clients[MAX_CLIENTS]; // Array that hold info for all connected clients
static int client_count = 0;   // How many clients are currently connected
static CRITICAL_SECTION clients_mutex;  // Mutex to protect client array

// Get current time and formats as [HH:MM:SS] for chat messages
void getCurrentTimestamp(char *output) { 
    time_t raw_time;  
    struct tm *time_info;  // Broken down time
    time(&raw_time);  // Get current time
    time_info = localtime(&raw_time);  
    
    sprintf(output, "[%02d:%02d:%02d]",
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec);
}

// Send a message to ALL connected clients except the sender
void broadcastMessage(const char *message, SOCKET sender_socket) { 
    int i;
    
    EnterCriticalSection(&clients_mutex); // Look the mutex to ensure only one thread can be in this section a time
    
    // Loop through all connected clients
    for (i = 0; i < client_count; i++) {
        // Send everyone except the sender and invalid sockets
        if (clients[i].socket != sender_socket && clients[i].socket != INVALID_SOCKET) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    
    // Unlock the mutex
    LeaveCriticalSection(&clients_mutex);
}

// Remove client from client array when they disconnect
void removeClient(SOCKET socket) {
    int i, j;
    
    // Modify shared data
    EnterCriticalSection(&clients_mutex);
    
    // Find client with matching socket
    for (i = 0; i < client_count; i++) {
        if (clients[i].socket == socket) {
            // Shift remaining clients 
            // Move each client one position left
            for (j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    
    LeaveCriticalSection(&clients_mutex);
}

// Run in seperate thread for each client
DWORD WINAPI handleClient(LPVOID arg) {
    // Cast the void pointer back to ClientInfo pointer
    ClientInfo *client = (ClientInfo*)arg;
    char buffer[BUFFER_SIZE];  // Recieved message
    char message[BUFFER_SIZE * 2];  // Formatted messages
    char timestamp[20]; 
    int bytes_received;
    int client_id;  // Local copy of client ID
    SOCKET client_socket;  // Local copy of socket
    
    // Store client info locally to avoid issues after free 
    client_id = client->id;
    client_socket = client->socket;
    
    printf("Client %d (%s:%d) connected successfully!\n", 
           client->id, client->ip, client->port);
    
    // Send welcome message to the client 
    sprintf(message, "Welcome to the chat server! You are Client %d.\r\n", client->id);
    send(client->socket, message, strlen(message), 0);
    
    // Notify other clients that someone joined
    getCurrentTimestamp(timestamp);
    sprintf(message, "%s [Server]: Client %d has joined the chat.\r\n", timestamp, client->id);
    broadcastMessage(message, client->socket);
    
    // Loop run forever to recieve message from this specific client
    // Only will breaks and thread when client disconnect
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);  // Clear buffer
        // Recieve message from this client
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            // Client disconnected gracefully or error
            printf("Client %d disconnected\n", client_id);
            break;  // Exit loop and thread will end
        }
        
        buffer[bytes_received] = '\0';  // Null terminate
        
        // Remove trailing newline if present 
        if (buffer[bytes_received - 1] == '\n') {
            buffer[bytes_received - 1] = '\0';
            // Chieck for \r\n (Windows style)
            if (bytes_received > 1 && buffer[bytes_received - 2] == '\r') {
                buffer[bytes_received - 2] = '\0';
            }
        }
        
        // Print to server console
        printf("Client %d: %s\n", client_id, buffer);
        
        // Check for exit command 
        if (strcmp(buffer, "Exit Server") == 0) {
            printf("Client %d sent exit command. Disconnecting...\n", client_id);
            
            // Notify the client 
            sprintf(message, "You have disconnected from the server. Goodbye!\r\n");
            send(client_socket, message, strlen(message), 0);
            
            // Notify other clients 
            getCurrentTimestamp(timestamp);
            sprintf(message, "%s [Server]: Client %d has left the chat.\r\n", timestamp, client_id);
            broadcastMessage(message, client_socket);
            break;
        }
        
        // Format and broadcast the message to other clients
        getCurrentTimestamp(timestamp);
        sprintf(message, "%s [Client %d]: %s\r\n", timestamp, client_id, buffer);
        broadcastMessage(message, client_socket);
    }
    
    // Clean up when thread is about to exit
    closesocket(client_socket);
    removeClient(client_socket);
    free(client);
    
    printf("Client %d handler thread terminated\n", client_id);
    
    return 0;  // Thread exit code
}

// Create server socket and spawn a new thread for each connectiong client
int main(int argc, char *argv[]) {
    SOCKET server_fd, client_socket;
    struct sockaddr_in address, client_addr;
    int addrlen = sizeof(client_addr);
    HANDLE thread;
    int opt = 1;
    ClientInfo *new_client;  // New pointer to dynamically allocated client info
    WSADATA wsaData;
    
    // Initialize Winsock 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Initialize mutex 
    InitializeCriticalSection(&clients_mutex);
    
    // Create socket 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    
    // Set socket option to reuse address 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        printf("Setsockopt failed\n");
    }
    
    // Configure server address structure 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket to IP and port 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Binding failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    // Listen for incoming connections 
    if (listen(server_fd, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listening failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    printf("========================================\n");
    printf("Chat Server Started\n");
    printf("========================================\n");
    printf("Server is listening on port %d...\n", PORT);
    printf("Waiting for clients to connect (Maximum %d clients)...\n\n", MAX_CLIENTS);
    
    // Accept connections in a loop 
    while (1) {
        // Accept a new connection 
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        
        if (client_socket == INVALID_SOCKET) {
            printf("Accepting connection failed\n");
            continue;  // Try again
        }
        
        // Check if maximum clients reached 
        if (client_count >= MAX_CLIENTS) {
            printf("Maximum clients reached. Rejecting new connection.\n");
            send(client_socket, "Server is full. Please try again later.\r\n", 42, 0);
            closesocket(client_socket);
            continue;
        }
        
        // Allocate memory for new client info 
        new_client = (ClientInfo*)malloc(sizeof(ClientInfo));
        if (new_client == NULL) {
            printf("Memory allocation failed\n");
            closesocket(client_socket);
            continue;
        }
        
        // Initialize client info 
        new_client->socket = client_socket;
        strcpy(new_client->ip, inet_ntoa(client_addr.sin_addr));
        new_client->port = ntohs(client_addr.sin_port);
        new_client->id = client_count + 1;  // Assign next available ID
        
        // Add to clients array 
        EnterCriticalSection(&clients_mutex);
        clients[client_count] = *new_client;  // Copy structure to array
        client_count++;  // Increment count
        LeaveCriticalSection(&clients_mutex);
        
        printf("\n========================================\n");
        printf("New Connection:\n");
        printf("Client IP: %s\n", new_client->ip);
        printf("Client Port: %d\n", new_client->port);
        printf("Assigned ID: %d\n", new_client->id);
        printf("Total Clients: %d/%d\n", client_count, MAX_CLIENTS);
        printf("========================================\n\n");
        
        // Create thread to handle this client 
        thread = CreateThread(NULL, 0, handleClient, new_client, 0, NULL);
        if (thread == NULL) {
            printf("Thread creation failed\n");
            closesocket(client_socket);
            removeClient(client_socket);
            free(new_client);  // clean up allocated memory
        } else {
            // Close thread handle as we don't need to wait for it 
            CloseHandle(thread);
        }
    }
    
    // Close server socket and cleanup 
    closesocket(server_fd);
    DeleteCriticalSection(&clients_mutex);  // clean up mutex
    WSACleanup();
    
    printf("Server terminated.\n");
    return 0;
}