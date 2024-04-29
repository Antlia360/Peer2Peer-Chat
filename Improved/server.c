#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <dirent.h>
#define PORT 5548
#define MAX_CLIENTS 10

// Struct to hold client info
struct client_info {
    int socket;
    char id[37]; //len 36 and for'\0'
};



// Array to store client info
struct client_info clients[MAX_CLIENTS];
int client_count = 0;

// Mutex for  thread safety
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to convert UUID to string
//void uuid_to_string(uuid_t uuid, char *str) {
 //   uuid_unparse_lower(uuid, str);
//}

int  delete_files_in_directory(const char *dir_name) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dir_name);
    if (dir == NULL) {
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

            if (remove(path) != 0) {
                perror("Error deleting file");
            } else {
                printf("Deleted file: %s\n", path);
            }
        }
    }

    closedir(dir);
    return 0;
}

// send list of active clients to a specific clients
void send_active_clients(int client_socket) {
    char response[1024];
    strcpy(response, "Active clients:\n");
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        sprintf(response + strlen(response), "ID: %s, Socket: %d\n", clients[i].id, clients[i].socket);
    }
    pthread_mutex_unlock(&mutex);
    send(client_socket, response, strlen(response), 0);
}

// broadcast message to all clients
void broadcast(char *message, int sender_index) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (i != sender_index) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// find client index by ID
int find_client_index(char *id) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

// handle client communication
void handle_client(void *arg) {
    int client_index =  *((int*)arg);
    char buffer[1024];

    while (1) {
        int bytes_received = recv(clients[client_index].socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Client %s disconnected\n", clients[client_index].id);
            close(clients[client_index].socket);

            pthread_mutex_lock(&mutex);
            for (int i = client_index; i < client_count - 1; i++) {
                clients[i] = clients[i + 1];
            }
            client_count--;
            pthread_mutex_unlock(&mutex);

            pthread_exit(NULL);
        }
        buffer[bytes_received] = '\0';
        printf("Received from client %s: %s\n", clients[client_index].id, buffer);

        if (strncmp(buffer, "/active", 7) == 0) {  //active handle
            send_active_clients(clients[client_index].socket);
        }  else if (strncmp(buffer, "/send", 5) == 0) {   //send handle
            struct stat st,st1;
            const char *dir=clients[client_index].id;
            if(!(stat(dir, &st) == 0  && S_ISDIR(st.st_mode))){
                int status = mkdir(dir, 0777);
            }
            char dest_id[37],  message[1024];
            sscanf(buffer, "/send %s %[^\n]", dest_id, message);

            const char *dir1=dest_id;
            if(!(stat(dir1,&st1) == 0 && S_ISDIR(st1.st_mode))){
                int status = mkdir(dir1, 0777);
            }
            int dest_index = -1;
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                if (strcmp(clients[i].id, dest_id) == 0) {
                    dest_index = i;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            if (dest_index == -1) {
                send(clients[client_index].socket, "Error: Destination client not found\n", 36, 0);
            } else{
                char message_with_id[1080];
                char message_with_id1[1080];
                char message_with_id2[1080];
                char file_path[150];
                char file_path1[150];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir, dest_id);
                snprintf(file_path1,  sizeof(file_path), "%s/%s", dir1,clients[client_index].id );
                FILE *file_write = fopen(file_path, "a"); 
                FILE *file_write1 = fopen(file_path1, "a"); 
                if (file_write == NULL || file_write1 == NULL) {
                    perror("Error opening file for writing");
                    break;
                }
                sprintf(message_with_id, "From %s: %s", clients[client_index].id, message);
                sprintf(message_with_id1, "%s\n",  message);
                sprintf(message_with_id2, "From %s: %s\n", clients[client_index].id, message);
                fputs(message_with_id1, file_write);
                fclose(file_write);
                fputs(message_with_id2, file_write1);
                fclose(file_write1);
                send(clients[dest_index].socket, message_with_id, strlen(message_with_id), 0);
            }
        } else if (strncmp(buffer, "/logout", 7) == 0) {

            send(clients[client_index].socket, "Bye!! Have a nice day\n", 22, 0);
            printf("Client %s disconnected\n", clients[client_index].id);
            close(clients[client_index].socket);

            pthread_mutex_lock(&mutex);
            for (int i = client_index; i < client_count - 1; i++) {
                clients[i] = clients[i + 1];
            }
            client_count--;
            pthread_mutex_unlock(&mutex);

            // pthread_exit(NULL);
                    
            break; 
        }else if (strncmp(buffer, "/history_delete", 15) == 0) {

            const char *dir=clients[client_index].id;
            char file_path[150];
            char dest_id[37];
            sscanf(buffer, "/history_delete %s", dest_id);          ///history_delete
            snprintf(file_path, sizeof(file_path), "%s/%s", dir, dest_id);
            if (remove(file_path) == 0) {
                send(clients[client_index].socket, "History deleted successfully.\n", 30, 0);
            } else {
                send(clients[client_index].socket, "History for given recepent not exist!\n", 38, 0);
            }
        }
        else if (strncmp(buffer, "/delete_all", 11) == 0) {   //"/delete_all"

            const char *dir=clients[client_index].id;
            if(delete_files_in_directory(dir)){
                send(clients[client_index].socket, "Chats deleted successfully.\n", 28, 0);
            }
            else{
                if (rmdir(dir) == 0) {
                    send(clients[client_index].socket, "Chats deleted successfully.\n", 28, 0);
                } else {
                    send(clients[client_index].socket, "Chats deleted successfully.\n", 28, 0);
                }
            }
            
        }
        
        else if (strncmp(buffer, "/history", 8) == 0) {    //history handle

            const char *dir=clients[client_index].id;
            char file_path[150];
            char dest_id[37];
            char message_with_id[1080];
            sscanf(buffer, "/history %s", dest_id);
            snprintf(file_path, sizeof(file_path), "%s/%s", dir, dest_id);
            FILE *file = fopen(file_path, "r");
            if (file == NULL) {
                send(clients[client_index].socket, "no such receipent in chat\n", 26, 0);
                fclose(file);
            }
            else{
            while (fgets(message_with_id, sizeof(message_with_id), file)) {
            // Send the line here (replace this with your sending logic)
                send(clients[client_index].socket, message_with_id, strlen(message_with_id), 0);
            }
            fclose(file);
            } 
        }
        
         else {
            broadcast(buffer, client_index);
        }
    }
}


int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;
    int client_addr_len = sizeof(client_addr);

    // Create  server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket  == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Bind socket to serveraddress 
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for  incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    while (1) {
            client_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len);
            if (client_socket == -1) {
                perror("Accept failed");
                continue;
            }

            char client_id[37];
            uuid_t uuid;
            uuid_generate(uuid);
            uuid_unparse(uuid, client_id);

            pthread_mutex_lock(&mutex);
            clients[client_count].socket = client_socket;
            strcpy(clients[client_count].id, client_id);
            client_count++;
            pthread_mutex_unlock(&mutex);

            printf("Client: %s Socket: %d connected \n", client_id , client_socket);
            send(client_socket, client_id, strlen(client_id), 0);

            int *arg = malloc(sizeof(*arg));
            *arg = client_count - 1;
            
            //function handle
            if (pthread_create(&tid,NULL, (void *)&handle_client, arg) != 0) {
                perror("Thread creation failed");
                continue;
            }
            pthread_detach(tid);
        }

    close(server_socket);
    return 0;
}
