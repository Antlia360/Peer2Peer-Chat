#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 5555
#define SERVER_IP "127.0.0.1"
#define MAX_CLIENTS 10

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    fd_set read_fds;
    char message[1024];

    // Create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // Connection to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server \n");

    while (1) {
        // printf("Enter command or message: ");
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); 
        FD_SET(client_socket, &read_fds);

        // Use select to monitor for input on stdin and client socket
        int activity = select(client_socket + 1, &read_fds, NULL, NULL, NULL);
        if (activity == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Check if there is input from stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // Get user input
            fgets(message, sizeof(message), stdin);

            // Remove trailing newline character
            message[strcspn(message, "\n")] = '\0';

            // Send the input to the server
            send(client_socket, message, strlen(message), 0);

            // Check if the input is "/logout" to exit
            if (strcmp(message, "/logout") == 0) {
                printf("Logging out. Goodbye!\n");
                break;
            }
        }

        // Check if there is incoming message from the server
        if (FD_ISSET(client_socket, &read_fds)) {
            // Receive and print response from the server
            int bytes_received = recv(client_socket, message, sizeof(message), 0);
            if (bytes_received == -1) {
                perror("Receive failed");
                exit(EXIT_FAILURE);
            } else if (bytes_received == 0) {
                printf("Server disconnected. Exiting.\n");
                break;
            }
            message[bytes_received] = '\0';
            printf("%s\n", message);
        }
    }

    close(client_socket);
    return 0;
}
