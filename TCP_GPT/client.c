#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h> 

#define BUFFER_SIZE 10240

int main() {
    char *ip = "127.0.0.1";
    int port = 5566;

    int sock;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE], buffer1[BUFFER_SIZE];

    int active_flag=0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP client socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[-]Connection error");
        exit(1);
    }
    printf("Connected to the server.\n");

    bzero(buffer, BUFFER_SIZE);
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Server: %s\n>", buffer);
    fflush(stdout);
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            bzero(buffer, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
            if(strncmp(buffer, "/chatbot_v2 login", strlen("/chatbot_v2 login")) == 0){
                active_flag=1;
            }
            if(strncmp(buffer, "/chatbot_v2 logout", strlen("/chatbot_v2 logout")) == 0){
                active_flag=0;
            }
            if(strncmp(buffer, "/logout", strlen("/logout")) == 0) {
                printf("Disconnected from the server.\n");
                break;
            }
            if(active_flag==0){
                printf("\n>");
                fflush(stdout);
            }
            
        }

        if (FD_ISSET(sock, &read_fds)) {
            bzero(buffer1, BUFFER_SIZE);
            sleep(5);
            int bytes_received = recv(sock, buffer1, sizeof(buffer1), 0);
            if (bytes_received > 0) {
                if(active_flag){
                    printf("%s\nUser> ", buffer1);
                    fflush(stdout);
                }
                else{
                    if (strncmp(buffer, "/history", strlen("/history")) == 0) {
                        char recipient_id[37];
                        sscanf(buffer, "/history %s", recipient_id);
                        printf("Retrieving chat history with %s...", recipient_id);
                        printf("\n%s\n>",buffer1);
                        fflush(stdout);
                    } else if (strncmp(buffer, "/history_delete", strlen("/history_delete")) == 0) {
                        char recipient_id[37];
                        sscanf(buffer, "/history_delete %s", recipient_id);
                        printf("Deleting chat history with %s...\n>", recipient_id);
                        printf("Deleted successfully...\n>");
                        fflush(stdout);
                    } else if (strncmp(buffer, "/delete_all", strlen("/delete_all")) == 0) {
                        printf("Deleting complete chat history...\n");
                        printf("Deleted successfully...\n>");
                        fflush(stdout);
                    }
                    else{
                        printf("%s\n>", buffer1);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    close(sock);
    return 0;
}
