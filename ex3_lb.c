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
#define END_OF_HTTP_REQUEST "\r\n\r\n"
#define LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST 4

void handle_connections(const int *http_socket, int server_sockets_list[3]);
void bind_random_port(int* http_socket, int* server_socket ,struct sockaddr_in* server_sockaddr, struct sockaddr_in* http_server_sockaddr);
void write_port_to_file(int random_port, char *port_type);
int get_random_port();
char* process_http_request_to_server(char* http_request_buffer, int *server_number, int server_sockets_list[3]);
void send_back_response_to_client(char* response_from_server, const int* http_socket);
void connect_with_servers(int *server_socket, int server_sockets_list[3]);

int main(){
    int http_socket = -1, server_socket = -1, opt = 1;
    struct sockaddr_in server_sockaddr = {0}, http_server_sockaddr = {0};
    int server_sockets_list[3];
    srand(time(0));

    http_socket   = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    setsockopt(http_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?
    opt = -1; // ?
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?

    bind_random_port(&http_socket, &server_socket, &server_sockaddr, &http_server_sockaddr);
    listen(http_socket, 10); // check for failure? change from 10 !
    listen(server_socket, 3); // check for failure? change from 10 !



    connect_with_servers(&server_socket, server_sockets_list);


    handle_connections(&http_socket, server_sockets_list); // dont care about peer?








    return EXIT_SUCCESS;
}


void connect_with_servers(int *server_socket, int server_sockets_list[3]){
    server_sockets_list[0] = accept(*server_socket, NULL, NULL);
    server_sockets_list[1] = accept(*server_socket, NULL, NULL);
    server_sockets_list[2] = accept(*server_socket, NULL, NULL);
}

void handle_connections(const int *http_socket, int server_sockets_list[3]){
    int connection_fd = -1, bytes_read, total_bytes_wrote = 0, server_number = 0;
    char* end_of_http_request;
    char buffer[RECEIVING_BUFFER_SIZE] = {0}; char *end_sequence_of_http_request_pointer, *response_from_server;
    char* http_request_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char)); // maybe not dynamically?
    while (true){
        connection_fd = accept(*http_socket, NULL, NULL);  // check for failure?,  TODO: change null to get http info to send back info to it?
        while (true){
            bytes_read = recv(connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0);
            if (bytes_read == -1){// remove ?
                printf("bytes read returned -1\n"); // remove ?
                break;
            }
            if ((end_sequence_of_http_request_pointer = strstr(buffer, END_OF_HTTP_REQUEST))!=NULL){ // means an end to an http request has come
                end_of_http_request = end_sequence_of_http_request_pointer + LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST;
                http_request_buffer = memcpy(http_request_buffer + total_bytes_wrote, buffer, end_of_http_request - buffer );
                printf("%s", http_request_buffer);
                //total_bytes_wrote  += end_of_http_request_pointer - buffer; // amount of bytes of the current http request
                response_from_server = process_http_request_to_server(http_request_buffer, &server_number, server_sockets_list);
                send_back_response_to_client(response_from_server, http_socket);
                memset(http_request_buffer, 0, RECEIVING_BUFFER_SIZE); free(response_from_server);
                memcpy(http_request_buffer, end_of_http_request, strlen(end_of_http_request));
                total_bytes_wrote = strlen(end_of_http_request);
                continue;
            }
            if (bytes_read == 0){ // TODO: check if got the end of http request, if not, continue, else, break ! even before or with no bytes_read==0
                printf("bytes read returned 0\n");
                printf("%s", http_request_buffer);
                break;
            }
            http_request_buffer=strncpy(http_request_buffer + total_bytes_wrote, buffer, bytes_read);
            total_bytes_wrote += bytes_read;
        }

    }
    free(http_request_buffer); // doesnt get here
}

char* process_http_request_to_server(char* http_request_buffer, int *server_number, int server_sockets_list[3]){
    send(server_sockets_list[*server_number], http_request_buffer, strlen(http_request_buffer), 0); // TODO: inside a loop?

    char* response_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    int bytes_read = recv(server_sockets_list[*server_number], response_buffer, RECEIVING_BUFFER_SIZE, 0); // TODO: this into loop? and untill reading two \r\n\r\n
    // TODO: check for if bytes_read==0 etc?

    *server_number = (*server_number + 1) % 3;
    return response_buffer;
}

void send_back_response_to_client(char* response_from_server, const int* http_socket){
    send(*http_socket, response_from_server, strlen(response_from_server), 0); // TODO: in a loop?
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
                (*server_sockaddr).sin_port = htons(random_port);
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