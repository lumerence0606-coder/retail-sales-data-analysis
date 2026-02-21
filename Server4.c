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
#define INVALID_TIMEZONE 999
#define IP_ADDR_LEN 16  //Length for IPv4 address string (xxx.xxx.xxx.xxx\0) 

// Structure to hold timezone information 
typedef struct {
    const char *code;  // Timezone code
    int offset;  // Offset Malaysia time
    const char *name;  // Full name
} TimezoneInfo;

// Timezone table - ALL offsets are relative to Malaysia time (GMT+8) 
static const TimezoneInfo timezones[] = {
    {"PST", -8, "Pacific Standard Time"},
    {"MST", -7, "Mountain Standard Time"},
    {"CST", -6, "Central Standard Time"},
    {"EST", -5, "Eastern Standard Time"},
    {"GMT", 0, "Greenwich Mean Time"},       // GMT in this context means Malaysia time
    {"CET", 1, "Central European Time"},
    {"MSK", 3, "Moscow Time"},
    {"JST", 9, "Japan Standard Time"},  // 1 hour ahead of Malysia
    {"AEDT", 11, "Australian Eastern Daylight Time"},
    {NULL, 0, NULL}  // Sentinel to mark end to array
};

// Validates input character
int isAlphanumeric(const char *str) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (!isalnum((unsigned char)str[i]) && str[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

// Look up a timezone code and return it offset
int getTimezoneOffset(const char *tz_code) {
    int i;
    // Loop through timezone table until hitting the NULL sentinel
    for (i = 0; timezones[i].code != NULL; i++) {
        if (strcmp(tz_code, timezones[i].code) == 0) {  // Found match
            return timezones[i].offset;
        }
    }
    return INVALID_TIMEZONE;  // Not found
}

// Calculate time in a specific timezone
void getTimeInTimezone(char *output, int offset_from_malaysia) {
    time_t raw_time;
    struct tm *time_info;
    
    // Get current time 
    time(&raw_time);
    
    // Adjust to Malaysia time (GMT+8) 
    raw_time += (MALAYSIA_GMT_OFFSET * 3600);  // Add 8 hour
    
    // Apply the offset from Malaysia time 
    raw_time += (offset_from_malaysia * 3600);  // Add timezone offset
    
    // Convert to UTC structure 
    time_info = gmtime(&raw_time);
    
    // Format output string 
    sprintf(output, "%02d:%02d:%02d\r\n",
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec);
}

// Get cirrent date or time in Malaysia timezone (same as server3)
void getCurrentDateTime(char *output) {
    time_t raw_time;
    struct tm *time_info;
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // Get current time 
    time(&raw_time);
    
    // Adjust to Malaysia time (GMT+8) 
    raw_time += (MALAYSIA_GMT_OFFSET * 3600);
    
    // Convert to UTC structure 
    time_info = gmtime(&raw_time);
    
    sprintf(output, "%s %s %02d %02d:%02d:%02d %d GMT+8\r\n",
            days[time_info->tm_wday],
            months[time_info->tm_mon],
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            time_info->tm_year + 1900);
}

// Parses command like "Time" or "Time GMT"
int parseTimeCommand(const char *input, char *timezone) {
    char cmd[BUFFER_SIZE];  // Hold the command word
    char tz[BUFFER_SIZE];  // Hold the timezone code
    int items;  // Number of items parsed
    
    // Parse command 
    items = sscanf(input, "%s %s", cmd, tz);
    
    if (items >= 1 && strcmp(cmd, "Time") == 0) {
        if (items == 1) {
            return 1;  // Just "Time" with no timezone
        } else if (items == 2) {
            strcpy(timezone, tz);
            return 2;  // "Time <TIMEZONE>"
        }
    }
    
    return 0;  // Not a time command
}

// Validates command which include those with arguments
int isValidCommand(const char *str) {
    char cmd[BUFFER_SIZE];
    char tz[BUFFER_SIZE];
    int items;
    
    // Check for "Date" command 
    if (strcmp(str, "Date") == 0) {
        return 1;
    }
    
    // Check for "Time" commands 
    items = sscanf(str, "%s %s", cmd, tz);
    if (items >= 1 && strcmp(cmd, "Time") == 0) {
        if (items == 1) {
            return 1;  // Just "Time" is valid
        } else if (items == 2) {
            // Validate timezone 
            if (getTimezoneOffset(tz) != INVALID_TIMEZONE) {
                return 1;
            }
        }
    }
    
    return 0;
}

// Validate for invalid character
// Validate for invalid command
void handleClient(SOCKET client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char timezone[BUFFER_SIZE];
    char client_ip[IP_ADDR_LEN];
    int client_port;
    int bytes_received;
    int time_cmd;  // Result for parseTimeCommand
    int tz_offset;
    
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
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);
        memset(timezone, 0, BUFFER_SIZE);  // Clear timezone buffer
        
        // Receive data from client 
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            printf("Client %s disconnected\n", client_ip);
            break;
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
            send(client_socket, response, strlen(response), 0);
            closesocket(client_socket);
            return;
        }
        
        // Process "Date" command 
        if (strcmp(buffer, "Date") == 0) {
            getCurrentDateTime(response);
            send(client_socket, response, strlen(response), 0);
            printf("Date command processed. Response sent: %s", response);
            continue;
        }
        
        // Process "Time" commands 
        time_cmd = parseTimeCommand(buffer, timezone);
        
        if (time_cmd == 1) {
            // "Time" command - return Malaysia time (offset 0 from Malaysia) 
            getTimeInTimezone(response, 0);
            send(client_socket, response, strlen(response), 0);
            printf("Time command processed. Malaysia time (GMT+8) sent: %s", response);
            continue;
        } else if (time_cmd == 2) {
            // "Time TIMEZONE" command - get time for specific timezone
            tz_offset = getTimezoneOffset(timezone);  // Look at the timezone offset from the table
            
            if (tz_offset == INVALID_TIMEZONE) {
                // Timezone code not found in the table
                strcpy(response, "ERROR: Invalid timezone code! Valid codes are: PST, MST, CST, EST, GMT, CET, MSK, JST, AEDT.");
                send(client_socket, response, strlen(response), 0);
                printf("Error: Invalid timezone '%s'. Error message sent to client.\n\n", timezone);
                continue;  // Skip to next message
            }
            
            // Valid timezone - calculate time using the offset
            getTimeInTimezone(response, tz_offset);
            send(client_socket, response, strlen(response), 0);
            printf("Time command processed for %s timezone. Response sent: %s", timezone, response);
            continue;
        }
        
        // Check for invalid characters 
        if (!isAlphanumeric(buffer)) {
            strcpy(response, "ERROR: Invalid input! Only alphanumeric characters or valid commands (e.g., 'Date', 'Time', 'Time PST') are allowed.");
            send(client_socket, response, strlen(response), 0);
            printf("Error: Invalid characters detected. Error message sent to client.\n\n");
            continue;
        }
        
        // Check for invalid command 
        if (!isValidCommand(buffer)) {
            strcpy(response, "ERROR: Invalid command! Please use valid commands like 'Date', 'Time', or 'Time <TIMEZONE>' (e.g., 'Time PST', 'Time GMT').");
            send(client_socket, response, strlen(response), 0);
            printf("Error: Not a valid command. Error message sent to client.\n\n");
            continue;
        }
    }
    
    closesocket(client_socket);
}

// Print additional info about supported timezone (Same structure as server3)
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
    if (listen(server_fd, MAX_CONNECTIONS) == SOCKET_ERROR) {
        printf("Listening failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    // Display server information with all supported command
    printf("Server is listening on port %d...\n", PORT);
    printf("Supported commands: 'Date', 'Time', 'Time <TIMEZONE>', 'Exit Server'\n");
    printf("Available timezones: PST, MST, CST, EST, GMT, CET, MSK, JST, AEDT\n");
    printf("Note: 'Time' alone returns Malaysia time (GMT+8)\n");
    printf("Waiting for client connections...\n");
    
    // Accept connections in a loop 
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        
        if (client_socket == INVALID_SOCKET) {
            printf("Accepting connection failed\n");
            continue;
        }
        
        // Handle the client 
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