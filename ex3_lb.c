#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

    http_socket   = socket(AF_INET, SOCK_STREAM, 0); //  AF_INET, SOCK_STREAM, 0 check for failure?
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
    int connection_fd = -1, old_connection_fd = -1, bytes_read, total_bytes_wrote = 0, server_number = 0;
    char* end_of_http_request;
    char buffer[RECEIVING_BUFFER_SIZE] = {0}; char *end_sequence_of_http_request_pointer, *response_from_server;
    char* http_request_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char)); // maybe not dynamically?
    while (true){
        old_connection_fd = connection_fd;
        printf("l76\n");
        connection_fd = accept(*http_socket,NULL, NULL);  // check for failure?,  TODO: change null to get http info to send back info to it?
      printf("l78\n");
        if(old_connection_fd != -1) {
            close(old_connection_fd);
        }
        while (true){
            printf("l83\n");
            bytes_read = recv(connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0); // debug run_test.py when breakpoint is a bit after here
            printf("l85\n");
            if (bytes_read < RECEIVING_BUFFER_SIZE){
                memset(buffer + bytes_read, 0 ,RECEIVING_BUFFER_SIZE - bytes_read);
            }
            if (bytes_read == -1){// remove ?
                printf("bytes read returned -1\n"); // remove ?
                break;
            }
            if (bytes_read == 0){ // TODO: check if got the end of http request, if not, continue, else, break ! even before or with no bytes_read==0
                printf("bytes read returned 0\n");
                //memset(buffer, 0, RECEIVING_BUFFER_SIZE);
                //close(connection_fd);
                break;
            }
            strncpy(http_request_buffer + total_bytes_wrote, buffer, bytes_read);
            total_bytes_wrote += bytes_read;
            //if ((end_sequence_of_http_request_pointer = strstr(buffer, END_OF_HTTP_REQUEST))!=NULL){ // means an end to an http request has come
            if ((end_sequence_of_http_request_pointer = strstr(http_request_buffer, END_OF_HTTP_REQUEST))!=NULL){
                end_of_http_request = end_sequence_of_http_request_pointer + LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST;
                //memcpy(http_request_buffer + total_bytes_wrote, buffer, end_of_http_request - buffer );
                printf("%s", http_request_buffer);
                //total_bytes_wrote  += end_of_http_request_pointer - buffer; // amount of bytes of the current http request
                response_from_server = process_http_request_to_server(http_request_buffer, &server_number, server_sockets_list);
                send_back_response_to_client(response_from_server, &connection_fd);
                //send(connection_fd, response_from_server, strlen(response_from_server), 0);//send_back_response_to_client(response_from_server, http_socket);
                memset(http_request_buffer, 0, RECEIVING_BUFFER_SIZE); free(response_from_server);
                memcpy(http_request_buffer, end_of_http_request, strlen(end_of_http_request));
                total_bytes_wrote = strlen(end_of_http_request);
                printf("L110\n");
                continue;
                //break;
            }


        }

    }
    free(http_request_buffer); // doesnt get here
}

char* process_http_request_to_server(char* http_request_buffer, int *server_number, int server_sockets_list[3]){

    int total_bytes_sent = 0, total_bytes_to_send = strlen(http_request_buffer);
    while(total_bytes_sent < total_bytes_to_send){
        total_bytes_sent+=send(server_sockets_list[*server_number], http_request_buffer,strlen(http_request_buffer), 0);
        http_request_buffer+=total_bytes_sent;
    }

    char* response_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    recv(server_sockets_list[*server_number], response_buffer, RECEIVING_BUFFER_SIZE, 0); // TODO: this into loop? and untill reading two \r\n\r\n
    /*int total_bytes_read = 0, bytes_read;
    do {
      bytes_read = recv(server_sockets_list[*server_number], response_buffer + total_bytes_read,
          RECEIVING_BUFFER_SIZE - total_bytes_read, 0); // TODO:  untill reading two \r\n\r\n
      total_bytes_read+= bytes_read;
    }while(bytes_read > 0 && !strstr(response_buffer, END_OF_HTTP_REQUEST) );
*/
    // TODO: check for if bytes_read==0 etc?

    *server_number = (*server_number + 1) % 3;
    return response_buffer;
}

void send_back_response_to_client(char* response_from_server, const int* connection_fd){

    int total_bytes_sent = 0, total_bytes_to_send = strlen(response_from_server);
    while(total_bytes_sent < total_bytes_to_send){
      total_bytes_sent+=send(*connection_fd, response_from_server+total_bytes_sent,
          total_bytes_to_send - total_bytes_sent, 0);
    }
    shutdown(*connection_fd,SHUT_RDWR);
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
            break;
        }
    }
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