#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAXIMUM_PORT_RANGE 64000
#define STARTING_PORT_RANGE 1024
#define RECEIVING_BUFFER_SIZE 1024
#define HTTP_PORT "http_port"
#define SERVER_PORT "server_port"
#define END_OF_HTTP_REQUEST "\r\n\r\n"
#define LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST 4
#define CONNECTION_FD_INIT -1
#define NUMBER_OF_SERVERS 3
#define CLIENT_QUEUE_SIZE 1
#define TWO_OCCURENCES 2

void sockets_setup(int* http_socket, int* server_socket, struct sockaddr_in* server_sockaddr,
                   struct sockaddr_in*http_server_sockaddr);
void bind_random_ports(const int* http_socket, const int* server_socket, struct sockaddr_in* server_sockaddr,
                       struct sockaddr_in* http_server_sockaddr);
void bind_available_port(int socket, struct sockaddr_in* sockaddr, char* file_name);
int get_random_port();
void write_port_to_file(int random_port, char* port_type);
void connect_with_servers(int server_socket, int server_sockets_list[NUMBER_OF_SERVERS]);
void handle_connections(const int http_socket, int server_sockets_list[NUMBER_OF_SERVERS]);
bool end_of_http_request_twice_in_string(char* string);
char* process_http_request_to_server(char* http_request_buffer, int* server_number,
                                     int server_sockets_list[NUMBER_OF_SERVERS]);
void send_back_response_to_client(char* response_from_server, const int http_socket);
void accept_new_connection_and_drop_last(const int http_socket, int* new_connection_fd, int* old_connection_fd);
void get_http_request_and_send_response_to_client(const int new_connection_fd, int server_sockets_list[NUMBER_OF_SERVERS], int* server_number);

int main()
{
    int http_socket = CONNECTION_FD_INIT, server_socket = CONNECTION_FD_INIT;
    struct sockaddr_in server_sockaddr = {0}, http_server_sockaddr = {0};
    int server_sockets_list[NUMBER_OF_SERVERS];
    srand(time(0));
    sockets_setup(&http_socket, &server_socket, &server_sockaddr, &http_server_sockaddr);
    connect_with_servers(server_socket, server_sockets_list);
    handle_connections(http_socket, server_sockets_list);

    return EXIT_SUCCESS;
}

void sockets_setup(int* http_socket,int* server_socket, struct sockaddr_in* server_sockaddr,
                   struct sockaddr_in*http_server_sockaddr){
    int opt = 1;

    *http_socket = socket(AF_INET, SOCK_STREAM, 0);
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(*http_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind_random_ports(http_socket, server_socket, server_sockaddr, http_server_sockaddr);
    listen(*http_socket, CLIENT_QUEUE_SIZE);
    listen(*server_socket, NUMBER_OF_SERVERS);
}

void bind_random_ports(const int* http_socket, const int* server_socket, struct sockaddr_in* server_sockaddr,
                       struct sockaddr_in* http_server_sockaddr)
{
    int random_port;
    (*http_server_sockaddr).sin_family = AF_INET;
    (*http_server_sockaddr).sin_addr.s_addr = INADDR_ANY;
    (*server_sockaddr).sin_family = AF_INET;
    (*server_sockaddr).sin_addr.s_addr = INADDR_ANY;

    bind_available_port(*http_socket, http_server_sockaddr, HTTP_PORT);
    bind_available_port(*server_socket, server_sockaddr, SERVER_PORT);
}

void bind_available_port(int socket, struct sockaddr_in* sockaddr, char* file_name){

    while (true) {
        int random_port = get_random_port();
        (*sockaddr).sin_port = htons(random_port);
        if (bind(socket, (struct sockaddr*)sockaddr, sizeof(*sockaddr)) < 0) {
            continue;
        } else {
            write_port_to_file(random_port, file_name);
            break;
        }
    }
}

int get_random_port()
{
    int random_port = (rand() % (MAXIMUM_PORT_RANGE + 1 - STARTING_PORT_RANGE)) + STARTING_PORT_RANGE;
    return random_port;
}

void write_port_to_file(int random_port, char* port_type)
{
    FILE* fd;
    if (strcmp(port_type, HTTP_PORT) == 0) {
        fd = fopen(HTTP_PORT, "w");
    } else if (strcmp(port_type, SERVER_PORT) == 0) {
        fd = fopen(SERVER_PORT, "w");
    }
    fprintf(fd, "%d", random_port);
    fclose(fd);
}

void connect_with_servers(int server_socket, int server_sockets_list[NUMBER_OF_SERVERS])
{
    int server_index;
    for (server_index = 0 ; server_index < NUMBER_OF_SERVERS ; server_index++) {
        server_sockets_list[server_index] = accept(server_socket, NULL, NULL);
    }
}

void handle_connections(const int http_socket, int server_sockets_list[NUMBER_OF_SERVERS])
{
    int new_connection_fd = CONNECTION_FD_INIT, old_connection_fd, server_number = 0,




    while (true) {
        accept_new_connection_and_drop_last(http_socket, &new_connection_fd, &old_connection_fd);
        get_http_request_and_send_response_to_client(new_connection_fd, server_sockets_list, &server_number );
//        old_connection_fd = connection_fd;
//        connection_fd = accept(http_socket, NULL, NULL);
//        if (old_connection_fd != CONNECTION_FD_INIT) {
//            close(old_connection_fd);
//        }
//            bytes_read = recv(new_connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0);
//            if (bytes_read < RECEIVING_BUFFER_SIZE) {
//                memset(buffer + bytes_read, 0, RECEIVING_BUFFER_SIZE - bytes_read);
//            }
//            if (bytes_read <= 0) {
//                printf("Error in while receiving from socket.\n");
//                break;
//            }
//            if (total_bytes_wrote + bytes_read > http_request_buffer_size) {
//                http_request_buffer = (char*)realloc(http_request_buffer, http_request_buffer_size + RECEIVING_BUFFER_SIZE);
//                memset(http_request_buffer + http_request_buffer_size, 0, RECEIVING_BUFFER_SIZE);
//                http_request_buffer_size += RECEIVING_BUFFER_SIZE;
//            }
//
//            strncpy(http_request_buffer + total_bytes_wrote, buffer, bytes_read);
//            total_bytes_wrote += bytes_read;
//            if (strstr(http_request_buffer, END_OF_HTTP_REQUEST) != NULL) {
//                response_from_server = process_http_request_to_server(http_request_buffer, &server_number, server_sockets_list);
//                send_back_response_to_client(response_from_server, new_connection_fd);
//                memset(http_request_buffer, 0, http_request_buffer_size);
//                total_bytes_wrote = 0;
//                break;
//            }
    }
}

void get_http_request_and_send_response_to_client(const int new_connection_fd, int server_sockets_list[NUMBER_OF_SERVERS], int* server_number ) {
    int bytes_read, total_bytes_wrote = 0, http_request_buffer_size = RECEIVING_BUFFER_SIZE;
    char buffer[RECEIVING_BUFFER_SIZE] = {0};
    char* http_request_buffer          = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    char* response_from_server;

    while (true){
        bytes_read = recv(new_connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0);
        if (bytes_read < RECEIVING_BUFFER_SIZE) {
            memset(buffer + bytes_read, 0, RECEIVING_BUFFER_SIZE - bytes_read);
        }
        if (bytes_read <= 0) {
            printf("Error in while receiving from socket.\n");
            break;
        }
        if (total_bytes_wrote + bytes_read > http_request_buffer_size) {
            http_request_buffer = (char*)realloc(http_request_buffer, http_request_buffer_size + RECEIVING_BUFFER_SIZE);
            memset(http_request_buffer + http_request_buffer_size, 0, RECEIVING_BUFFER_SIZE);
            http_request_buffer_size += RECEIVING_BUFFER_SIZE;
        }

        strncpy(http_request_buffer + total_bytes_wrote, buffer, bytes_read);
        total_bytes_wrote += bytes_read;
        if (strstr(http_request_buffer, END_OF_HTTP_REQUEST) != NULL) {
            response_from_server = process_http_request_to_server(http_request_buffer, server_number, server_sockets_list);
            send_back_response_to_client(response_from_server, new_connection_fd);
            memset(http_request_buffer, 0, http_request_buffer_size);
            total_bytes_wrote = 0;
            break;
        }
    }
}

void accept_new_connection_and_drop_last(const int http_socket, int* new_connection_fd, int* old_connection_fd){
    *old_connection_fd = *new_connection_fd;
    *new_connection_fd = accept(http_socket, NULL, NULL);
    if (*old_connection_fd != CONNECTION_FD_INIT) {
        close(*old_connection_fd);
    }
}

bool end_of_http_request_twice_in_string(char* string)
{

    int offset_index = 0, string_length = strlen(string), occurrence_counter = 0;

    while (offset_index < string_length) {
        if (strncmp(END_OF_HTTP_REQUEST, string + offset_index, LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST) == 0) {
            occurrence_counter++;
            offset_index += LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST;
        } else {
            offset_index++;
        }
        if (occurrence_counter == TWO_OCCURENCES) {
            return true;
        }
    }
    return false;
}

char* process_http_request_to_server(char* http_request_buffer, int* server_number,
                                     int server_sockets_list[NUMBER_OF_SERVERS])
{

    int total_bytes_sent = 0, total_bytes_to_send = strlen(http_request_buffer), total_bytes_read = 0, bytes_read = 0,
            total_response_buffer_size = RECEIVING_BUFFER_SIZE;
    char* total_response_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    char buffer[RECEIVING_BUFFER_SIZE] = {0};

    while (total_bytes_sent < total_bytes_to_send) {
        total_bytes_sent += send(server_sockets_list[*server_number], http_request_buffer, strlen(http_request_buffer), 0);
        http_request_buffer += total_bytes_sent;
    }

    do {
        bytes_read = recv(server_sockets_list[*server_number], buffer, RECEIVING_BUFFER_SIZE, 0);
        if (total_bytes_read + bytes_read > total_response_buffer_size) {
            total_response_buffer = (char*)realloc(total_response_buffer, total_response_buffer_size + RECEIVING_BUFFER_SIZE);
            memset(total_response_buffer + total_response_buffer_size, 0, RECEIVING_BUFFER_SIZE);
            total_response_buffer_size += RECEIVING_BUFFER_SIZE;
        }
        if (bytes_read < RECEIVING_BUFFER_SIZE) {
            memset(buffer + bytes_read, 0, RECEIVING_BUFFER_SIZE - bytes_read);
        }
        strncpy(total_response_buffer + total_bytes_read, buffer, bytes_read);
        total_bytes_read += bytes_read;
    } while (!end_of_http_request_twice_in_string(total_response_buffer));

    *server_number = ((*server_number) + 1) % NUMBER_OF_SERVERS;
    return total_response_buffer;
}

void send_back_response_to_client(char* response_from_server, const int connection_fd)
{

    int total_bytes_sent = 0, total_bytes_to_send = strlen(response_from_server);

    while (total_bytes_sent < total_bytes_to_send) {
        total_bytes_sent +=
                send(connection_fd, response_from_server + total_bytes_sent, total_bytes_to_send - total_bytes_sent, 0);
    }
    shutdown(connection_fd, SHUT_RDWR);
    free(response_from_server);
}