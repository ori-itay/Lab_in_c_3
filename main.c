#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h> // check that all are neccessary
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXIMUM_PORT 64000
#define STARTING_RANGE_PORT 1024
#define RECEIVING_BUFFER_SIZE 1024
#define HTTP_PORT "http_port"
#define SERVER_PORT "server_port"

void handle_connections(int *socket);
void bind_random_port(int* http_socket, int* server_socket ,struct sockaddr_in* server_sockaddr, struct sockaddr_in* http_server_sockaddr);
void write_port_to_file(int random_port, char *port_type);
int get_random_port();

int main(){
    int http_socket = -1, server_socket = -1, opt = 1;
    struct sockaddr_in server_sockaddr = {0}, http_server_sockaddr = {0};
    srand(time(0));


    http_socket   = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    setsockopt(http_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?
    opt = -1; // ?
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?

    bind_random_port(&http_socket, &server_socket, &server_sockaddr, &http_server_sockaddr);
    listen(http_socket, 10); // check for failure? change from 10 !
    listen(server_socket, 3);


    handle_connections(&http_socket); // dont care about peer?








    return EXIT_SUCCESS;
}


void handle_connections(int *socket){
    int connection_fd = -1, bytes_read, total_bytes_wrote = 0, times_seen_end_of_http_request = 0;
    char buffer[RECEIVING_BUFFER_SIZE] = {0};
    char* http_request_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    while (true){
        connection_fd = accept(*socket, NULL, NULL);  // check for failure?,  TODO: change null to get http info to send back info to it?
        while (true){
            bytes_read = recv(connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0);
            if (bytes_read == -1){// remove ?
                printf("bytes read returned -1\n"); // remove ?
                break;
            }
            //printf("%s", buffer);
            if (bytes_read == 0){ // TODO: check if got the end of http request, if not, continue, else, break ! even before or with no bytes_read==0
                printf("bytes read returned 0\n");
                printf("%s", http_request_buffer);
                break;
            }
            total_bytes_wrote += bytes_read;
            http_request_buffer=strncpy(http_request_buffer + total_bytes_wrote, buffer, bytes_read);
        }

    }
    free(http_request_buffer); // doesnt get here
}


void bind_random_port(int* http_socket, int* server_socket ,struct sockaddr_in* server_sockaddr, struct sockaddr_in* http_server_sockaddr){
    int random_port;
    (*http_server_sockaddr).sin_family = AF_INET;
    (*http_server_sockaddr).sin_addr.s_addr =  INADDR_ANY;
    (*server_sockaddr).sin_family = AF_INET;
    (*server_sockaddr).sin_addr.s_addr = INADDR_ANY;     // not sure who initiate the connection..I think LB sends to the servers first, if so, why he must set a listen

    while (true){
        int random_port = get_random_port();
        (*http_server_sockaddr).sin_port = htons(random_port);
        if (bind(*http_socket, (struct sockaddr*)http_server_sockaddr, sizeof(*http_server_sockaddr)) < 0 ){
            continue;
        }
        else {
            write_port_to_file(random_port, HTTP_PORT);
            while (true){
                random_port = get_random_port();
                if (bind(*server_socket, (struct sockaddr*)server_sockaddr, sizeof(*server_sockaddr))  < 0  ){
                    continue;
                }
                else {
                    write_port_to_file(random_port, SERVER_PORT);
                    return;
                }
            }
        }
    }
}

int get_random_port(){
    int random_port = (rand() % (MAXIMUM_PORT + 1 - STARTING_RANGE_PORT)) + STARTING_RANGE_PORT;
    return random_port;
}

void write_port_to_file(int random_port, char *port_type){
    FILE* fd;
    if ( strcmp(port_type, HTTP_PORT) ==0 ) {
        fd = fopen(HTTP_PORT,  "w");
    }
    else if (strcmp(port_type, SERVER_PORT) == 0) {
        fd = fopen(SERVER_PORT, "w");
    }
    fprintf(fd, "%d", random_port);
    fclose(fd);
}