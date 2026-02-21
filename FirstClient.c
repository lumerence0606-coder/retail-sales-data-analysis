// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>  // For Threading

#pragma comment(lib, "ws2_32.lib")

// Constants 
#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

// Global variables 
static volatile int running = 1;  // Flag to control both threads
static SOCKET sock = 0;  // Socket shared by both threads
static CRITICAL_SECTION print_mutex;  

// Function runs in a seperate threads which continuosly recieve message from server and display them
DWORD WINAPI receiveMessages(LPVOID arg) {
    char buffer[BUFFER_SIZE];  // Buffer for recieve message
    int bytes_received;  // Number of bytes recieve
    // Check the income message 
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);  // Clear buffer
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            if (running) {
                EnterCriticalSection(&print_mutex);  // Lock for printing
                printf("\n[!] Connection lost to server.\n");
                printf("You: ");  // Keep prompt visible
                fflush(stdout);  // Force output to display
                LeaveCriticalSection(&print_mutex);
                running = 0;  // Signal main thread to exit
            }
            break;  // Exit loop, thread end
        }
        
        buffer[bytes_received] = '\0';  //Null-terminate recieved data
        
        // Print received message with proper formatting 
        EnterCriticalSection(&print_mutex);
        printf("\r");  // Clear current line 
        printf("%s", buffer);  // Print recieved message
        printf("You: ");  // Reprint prompt
        fflush(stdout);  // Force output(Don't wait for newline) 
        LeaveCriticalSection(&print_mutex);
    }
    
    return 0;  // Thread exit code
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;  // Server address structure
    char message[BUFFER_SIZE];  // Buffer for user input
    HANDLE recv_thread;  // Handle for recieve thread
    int len;  
    WSADATA wsaData;  // Winsock initialization data
    
    // Initialize Winsock 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Initialize mutex for synchronized printing 
    InitializeCriticalSection(&print_mutex);
    
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
    
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    // Connect to server 
    printf("========================================\n");
    printf("Connecting to Chat Server...\n");
    printf("========================================\n");
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Connection Failed. Make sure the server is running.\n");
        closesocket(sock);
        DeleteCriticalSection(&print_mutex);
        WSACleanup();
        return 1;
    }
    
    printf("Connected to server successfully!\n\n");
    printf("========================================\n");
    printf("Chat Instructions:\n");
    printf("========================================\n");
    printf("- Type your message and press Enter to send\n");
    printf("- Type 'Exit Server' to disconnect\n");
    printf("- Messages from other clients will appear automatically\n");
    printf("========================================\n\n");
    
    // Create thread to receive messages 
    recv_thread = CreateThread(NULL, 0, receiveMessages, NULL, 0, NULL);
    if (recv_thread == NULL) {
        printf("Thread creation failed\n");
        closesocket(sock);
        DeleteCriticalSection(&print_mutex);
        WSACleanup();
        return 1;
    }
    
    // Main loop to send messages 
    while (running) {
        EnterCriticalSection(&print_mutex);
        printf("You: ");
        fflush(stdout);
        LeaveCriticalSection(&print_mutex);
        
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove trailing newline 
        len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }
        
        // Check if message is empty 
        if (strlen(message) == 0) {
            continue;
        }
        
        // Send message to server 
        if (send(sock, message, strlen(message), 0) == SOCKET_ERROR) {
            EnterCriticalSection(&print_mutex);
            printf("Send failed\n");
            LeaveCriticalSection(&print_mutex);
            break;
        }
        
        // Check if user wants to exit 
        if (strcmp(message, "Exit Server") == 0) {
            EnterCriticalSection(&print_mutex);
            printf("Disconnecting from server...\n");
            LeaveCriticalSection(&print_mutex);
            running = 0;
            break;
        }
    }
    
    // Signal thread to stop and wait for it 
    running = 0;
    Sleep(500);
    
    // Disable both sending and recieving
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    
    // Wait for receive thread to finish 
    WaitForSingleObject(recv_thread, 2000);
    CloseHandle(recv_thread);
    
    
    DeleteCriticalSection(&print_mutex);
    // Cleanup Winsock
    WSACleanup();
    
    printf("\nDisconnected from server.\n");
    printf("Thank you for using the chat client!\n");
    
    return 0;
}