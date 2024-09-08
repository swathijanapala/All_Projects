#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <sys/select.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define SHM "shared_memory.txt"
#define CHAT_HISTORY_FILE "chat_history.txt"

typedef struct {
    int socket;
    char uuid[37];
    int online;
    int chatbot_active;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0,count=0;
fd_set master_fds;
//char faq_buffer[FAQ_BUFFER_SIZE];

pthread_mutex_t mutex;

void send_message(char *sender_id, char *dest_id, char *message);
void send_active_clients(int client_sock);
void process_message(int i,char *sender_id, char *message, int sd);
void features(int i,char *sender_id, char *message, int sd);


void add_to_chat_history(char *sender_id, char *recipient_id, char *message) {
    FILE *file = fopen(CHAT_HISTORY_FILE, "a");
    if (file == NULL) {
        perror("[-]Error opening chat history file");
        exit(1);
    }
    fprintf(file, "%s,%s,%s\n", sender_id, recipient_id, message);
    fclose(file);
}

// Function to retrieve chat history between two clients
void retrieve_chat_history(char *sender_id, char *recipient_id,int sender_socket) {
    FILE *file = fopen(CHAT_HISTORY_FILE, "r");
    if (file == NULL) {
        perror("[-]Error opening chat history file");
        exit(1);
    }

    char line[BUFFER_SIZE];
    char history_buffer[BUFFER_SIZE];
    bzero(history_buffer, BUFFER_SIZE);

    while (fgets(line, sizeof(line), file) != NULL) {
        char *sender = strtok(line, ",");
        char *recipient = strtok(NULL, ",");
        char *message = strtok(NULL, ",");

        if ((strcmp(sender, sender_id) == 0 && strcmp(recipient, recipient_id) == 0) ||
            (strcmp(sender, recipient_id) == 0 && strcmp(recipient, sender_id) == 0)) {
                sprintf(history_buffer + strlen(history_buffer), "%s: %s\n", sender, message);
                
        }
    }

    fclose(file);
    send(sender_socket, history_buffer, strlen(history_buffer), 0);
}

// Function to delete chat history with a specific recipient
void delete_chat_history(char *sender_id, char *recipient_id) {
    FILE *file = fopen(CHAT_HISTORY_FILE, "r");
    if (file == NULL) {
        perror("[-]Error opening chat history file");
        exit(1);
    }

    FILE *temp_file = fopen("temp.txt", "w");
    if (temp_file == NULL) {
        perror("[-]Error creating temporary file");
        exit(1);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *sender = strtok(line, ",");
        char *recipient = strtok(NULL, ",");
        char *message = strtok(NULL, ",");

        if (!((strcmp(sender, sender_id) == 0 && strcmp(recipient, recipient_id) == 0) ||
              (strcmp(sender, recipient_id) == 0 && strcmp(recipient, sender_id) == 0))) {
            if (message){
                printf("%s: %s\n", sender, message);
                fprintf(temp_file, "%s,%s,%s\n", sender, recipient, message);
            }
        }
    }

    fclose(file);
    fclose(temp_file);

    if (remove(CHAT_HISTORY_FILE) != 0) {
        perror("[-]Error deleting chat history file");
        exit(1);
    }

    if (rename("temp.txt", CHAT_HISTORY_FILE) != 0) {
        perror("[-]Error renaming file");
        exit(1);
    }
}

void delete_all_chat_history(char *sender_id) {
    FILE *file = fopen(CHAT_HISTORY_FILE, "r");
    if (file == NULL) {
        perror("[-]Error opening chat history file");
        exit(1);
    }

    FILE *temp_file = fopen("temp.txt", "w");
    if (temp_file == NULL) {
        perror("[-]Error creating temporary file");
        exit(1);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *sender = strtok(line, ",");
        char *recipient = strtok(NULL, ",");
        char *message = strtok(NULL, ",");

        if (strcmp(sender, sender_id) != 0 && message){
            fprintf(temp_file, "%s,%s,%s", sender, recipient, message);
        }
    }

    fclose(file);
    fclose(temp_file);

    if (remove(CHAT_HISTORY_FILE) != 0) {
        perror("[-]Error deleting chat history file");
        exit(1);
    }

    if (rename("temp.txt", CHAT_HISTORY_FILE) != 0) {
        perror("[-]Error renaming file");
        exit(1);
    }
}

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;
    int server_sock, client_sock, max_sd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    FD_ZERO(&master_fds);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    listen(server_sock, 10);
    printf("Listening...\n");

    FD_SET(server_sock, &master_fds);
    max_sd = server_sock;

    while (1) {
        fd_set read_fds = master_fds;
        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("[-]Select error");
            exit(1);
        }

        // Handle new connections
        if (FD_ISSET(server_sock, &read_fds)) {
            addr_size = sizeof(client_addr);
            client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
            if (client_sock < 0) {
                perror("[-]Accept error");
                exit(1);
            }
            printf("[+]Client connected.\n");

            uuid_t new_uuid;
            uuid_generate(new_uuid);
            char uuid_str[37];
            uuid_unparse(new_uuid, uuid_str);

            clients[client_count].socket = client_sock;
            strcpy(clients[client_count].uuid, uuid_str);
            clients[client_count].online = 1;
            clients[client_count].chatbot_active = 0;
            client_count++;

            FD_SET(client_sock, &master_fds);
            if (client_sock > max_sd) {
                max_sd = client_sock;
            }

            bzero(buffer, BUFFER_SIZE);
            sprintf(buffer, "Welcome to the chat! Your ID is: %s\n", clients[client_count - 1].uuid);
            send(client_sock, buffer, strlen(buffer), 0);
        }

        // Handle client activity
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket;
            if (FD_ISSET(sd, &read_fds)) {
                if (recv(sd, buffer, BUFFER_SIZE, 0) <= 0) {
                    printf("[+]Client %s disconnected.\n", clients[i].uuid);
                    close(sd);
                    FD_CLR(sd, &master_fds);
                    clients[i].online = 0;
                    clients[i].socket = -1;
                } else {
                    if (strncmp(buffer,"/chatbot_v2 login", strlen("/chatbot_v2 login")) == 0) {
                        clients[i].chatbot_active = 1;
                        char response[BUFFER_SIZE];
                        sprintf(response, "gpt2bot> Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n");
                        send(sd, response, strlen(response), 0);

                    } else if (strncmp(buffer, "/chatbot_v2 logout", strlen("/chatbot_v2 logout")) == 0) {
                        clients[i].chatbot_active = 0;
                        char response[BUFFER_SIZE];
                        sprintf(response, "gpt2bot> Bye! Have a nice day and do not complain about me.\n");
                        send(sd, response, strlen(response), 0);
                    }
                    else if(clients[i].chatbot_active==1){
                        char *newline_pos = strchr(buffer, '\n');
                        if(newline_pos != NULL) {
                            *newline_pos = '\0';
                            process_message(i,clients[i].uuid, buffer, sd);
                        }
                    }

                    else{
                        features(i,clients[i].uuid, buffer, sd);
                    }
                }
            }
        }
    }

    close(server_sock);
    return 0;
}


void send_message(char *sender_id, char *dest_id, char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(clients[i].uuid, dest_id) == 0) {
            if (clients[i].online) {
                send(clients[i].socket, message, strlen(message), 0);
            } else {
                char offline_msg[10000];
                sprintf(offline_msg, "Error: Client %s is offline. Message not delivered.\n", dest_id);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (strcmp(clients[j].uuid, sender_id) == 0) {
                        send(clients[j].socket, offline_msg, strlen(offline_msg), 0);
                        break;
                    }
                }
            }
            break;
        }
    }
}

void send_active_clients(int client_sock) {
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "Active clients:\n");
    for (int i = 0; i < client_count; i++) {
        if (clients[i].online) {
            sprintf(buffer + strlen(buffer), "UUID: %s\n", clients[i].uuid);
        }
    }
    send(client_sock, buffer, strlen(buffer), 0);
}

void features(int i,char *sender_id, char *message, int sd){
    char m[100];
    strcpy(m,message);
    char *token = strtok(m, " ");
    if (strncmp(message, "/active", strlen("/active")) == 0) {
        send_active_clients(sd);
        return;
    }
    else if (strncmp(message, "/send", strlen("/send")) == 0) {
        char dest_id[37], sender[37];
        char msg_buffer[BUFFER_SIZE];
        sscanf(message, "/send %s %[^\n]", dest_id, msg_buffer);
        strcpy(sender, sender_id);
        add_to_chat_history(sender_id,dest_id,msg_buffer);
        send_message(sender_id, dest_id, msg_buffer);
        return;
    }
    else if ((strncmp(message, "/logout", strlen("/logout")) == 0) || (clients[i].online == 0)) {
        for (int j = 0; j < client_count; j++) {
            if (clients[j].socket == sd) {
                // Notify other clients about this client's exit
                char logout_msg[BUFFER_SIZE];
                sprintf(logout_msg, "Client %s left the chat.\n", clients[j].uuid);
                for (int k = 0; k < client_count; k++) {
                    if (k != j && clients[k].socket != -1) {
                        send(clients[k].socket, logout_msg, strlen(logout_msg), 0);
                    }
                }
                close(sd);
                FD_CLR(sd, &master_fds);
                clients[j].socket = -1;
                clients[j].online = 0;
                return;
            }
        }
    } else if ((token != NULL) && (strcmp(token, "/history") == 0)) {
        char recipient_id[37];
        sscanf(message, "/history %s", recipient_id);
        retrieve_chat_history(sender_id, recipient_id,sd);
        return;
    }else if ((token != NULL) && (strcmp(token, "/history_delete") == 0)) {
        char recipient_id[37];
        sscanf(message, "/history_delete %s", recipient_id);
        delete_chat_history(sender_id, recipient_id);
        return;
    } else if (strncmp(message, "/delete_all", strlen("/delete_all")) == 0) {
        delete_all_chat_history(sender_id);
        return;
    }else {
        // Broadcast the message to all clients
        for (int j = 0; j < client_count; j++) {
            if (clients[j].socket != -1) {
                send(clients[j].socket, message, strlen(message), 0);
            }
        }
    }
}

void process_message(int i, char *sender_id, char *message, int sd) {
    if (clients[i].socket == sd && clients[i].chatbot_active == 1) {   
        size_t msg_len = strlen(message);
        if (msg_len > 0 && message[msg_len - 1] == '\n') {
            message[msg_len - 1] = '\0';
        }
        //printf("message: %s\n",message);

        FILE *file = fopen(SHM,"a+");
        if (file == NULL) {
            perror("[-]Error opening SHM file");
            exit(1);
        }
        
        pthread_mutex_lock(&mutex);
        count=count+1;
        fprintf(file,"%d|%s|%s\n",count,sender_id,message);
        pthread_mutex_unlock(&mutex);

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            dup2(fileno(file), STDOUT_FILENO);
            fclose(file);

            char count_str[20];
            sprintf(count_str,"%d",count);
            execlp("python3","python3","gpt_2_gen.py",count_str,NULL);
            perror("execlp");
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                char line[25600];
                int count_val;
                char sender[100];
                char message[1000];
                char response[10000];
                FILE *file = fopen("shared_memory.txt", "r");
                if (file == NULL) {
                    perror("Error opening file");
                    exit(1);
                }
                while (fgets(line, sizeof(line), file) != NULL) {
                    sscanf(line, "%d|%99[^|]|%999[^|]|%9999[^|]", &count_val,sender,message,response);
                    if (strcmp(sender,sender_id) == 0 ) {   //&& strcmp(msg,message) == 0
                        //printf("response:%d,%s\n",count_val,response);
                        send(sd,response,strlen(response), 0);
                        break;
                     }
                }
                fclose(file);
            }else {
                printf("GPT-2 process terminated abnormally\n");
            }
        }        
    }
}