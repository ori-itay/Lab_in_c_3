#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXIMUM_PORT_RANGE 64000
#define STARTING_PORT_RANGE 1024
#define RECEIVING_BUFFER_SIZE 1024
#define HTTP_PORT "http_port"
#define SERVER_PORT "server_port"
#define END_OF_HTTP_REQUEST "\r\n\r\n"
#define LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST 4
#define CONECCTION_FD_INIT -1

void bind_random_port(int* http_socket, int* server_socket ,struct sockaddr_in*
    server_sockaddr, struct sockaddr_in* http_server_sockaddr);
int get_random_port();
void write_port_to_file(int random_port, char *port_type);
void connect_with_servers(int *server_socket, int server_sockets_list[3]);
void handle_connections(const int *http_socket, int server_sockets_list[3]);
bool end_of_http_request_twice_in_string(char *string);
char* process_http_request_to_server(char* http_request_buffer, int *server_number, int server_sockets_list[3]);
void send_back_response_to_client(char* response_from_server, const int* http_socket);

int main(){
    int http_socket = -1, server_socket = -1, opt = 1;
    struct sockaddr_in server_sockaddr = {0}, http_server_sockaddr = {0};
    int server_sockets_list[3];
    srand(time(0));

    http_socket   = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    setsockopt(http_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?

    bind_random_port(&http_socket, &server_socket, &server_sockaddr, &http_server_sockaddr);
    listen(http_socket, 10); // check for failure? change from 10 !
    listen(server_socket, 3); // check for failure? change from 10 !

    connect_with_servers(&server_socket, server_sockets_list);
    handle_connections(&http_socket, server_sockets_list); // dont care about peer?

    return EXIT_SUCCESS;
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
  int random_port = (rand() % (MAXIMUM_PORT_RANGE + 1 - STARTING_PORT_RANGE)) + STARTING_PORT_RANGE;
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

void connect_with_servers(int *server_socket, int server_sockets_list[3]){
    server_sockets_list[0] = accept(*server_socket, NULL, NULL);
    server_sockets_list[1] = accept(*server_socket, NULL, NULL);
    server_sockets_list[2] = accept(*server_socket, NULL, NULL);
}

void handle_connections(const int *http_socket, int server_sockets_list[3]){
    int connection_fd = CONECCTION_FD_INIT, old_connection_fd, bytes_read, total_bytes_wrote = 0, server_number = 0,
        http_request_buffer_size = RECEIVING_BUFFER_SIZE;
    char* end_of_http_request;
    char receive_buffer[RECEIVING_BUFFER_SIZE] = {0}; char *end_sequence_of_http_request_pointer, *response_from_server;
    char* http_request_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char)); // maybe not dynamically?
    while (true){
        old_connection_fd = connection_fd;
        connection_fd = accept(*http_socket,NULL, NULL);  // check for failure?
        if(old_connection_fd != CONECCTION_FD_INIT) {
            close(old_connection_fd);
        }
        while (true){
            bytes_read = recv(connection_fd, receive_buffer, RECEIVING_BUFFER_SIZE, 0);
            if (bytes_read < RECEIVING_BUFFER_SIZE){
                memset(receive_buffer + bytes_read, 0 ,RECEIVING_BUFFER_SIZE - bytes_read);
            }
            if (bytes_read <= 0){
                printf("Error in while receiving from socket.\n");
                break;
            }
            if(total_bytes_wrote+bytes_read > http_request_buffer_size){
              http_request_buffer = (char*) realloc(http_request_buffer, total_bytes_wrote + RECEIVING_BUFFER_SIZE);
              http_request_buffer_size+= RECEIVING_BUFFER_SIZE;
            }

            strncpy(http_request_buffer + total_bytes_wrote, receive_buffer, bytes_read);
            total_bytes_wrote += bytes_read;
            if ((end_sequence_of_http_request_pointer = strstr(http_request_buffer, END_OF_HTTP_REQUEST))!=NULL) { // means an end to an http request has come
              end_of_http_request = end_sequence_of_http_request_pointer +
                                    LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST;
              response_from_server = process_http_request_to_server(
                  http_request_buffer, &server_number, server_sockets_list);
              send_back_response_to_client(response_from_server, &connection_fd);
              memset(http_request_buffer, 0, total_bytes_wrote);
              memcpy(http_request_buffer, end_of_http_request, strlen(end_of_http_request));
              total_bytes_wrote = strlen(end_of_http_request); //some kind of initialization??
              break;
            }
        }
    }
    free(http_request_buffer); // doesnt get here - remove?
}

bool end_of_http_request_twice_in_string(char *string){

  int offset_index = 0, string_length = strlen(string), occurrence_counter = 0;

  while(offset_index < string_length){
    if(strncmp(END_OF_HTTP_REQUEST, string+offset_index, LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST) == 0){
      occurrence_counter++;
      offset_index+=LENGTH_OF_END_SEQUENCE_OF_HTTP_REQUEST;
    }
    else{
      offset_index++;
    }
    if(occurrence_counter == 2) {
      return true;
    }
  }
  return false;
}

char* process_http_request_to_server(char* http_request_buffer, int *server_number, int server_sockets_list[3]){

    int total_bytes_sent = 0, total_bytes_to_send = strlen(http_request_buffer), total_bytes_read = 0, bytes_read = 0,
        total_respone_buffer_length = RECEIVING_BUFFER_SIZE;
    char* total_respone_buffer = (char*)calloc(RECEIVING_BUFFER_SIZE, sizeof(char));
    char receive_buffer[RECEIVING_BUFFER_SIZE];

    while(total_bytes_sent < total_bytes_to_send){
        total_bytes_sent+=send(server_sockets_list[*server_number], http_request_buffer,strlen(http_request_buffer), 0);
        http_request_buffer+= total_bytes_sent;
    }

    do{
      bytes_read = recv(server_sockets_list[*server_number], receive_buffer, RECEIVING_BUFFER_SIZE, 0);
      if(total_bytes_read + bytes_read > total_respone_buffer_length){
        total_respone_buffer = (char*) realloc(http_request_buffer, total_bytes_read + RECEIVING_BUFFER_SIZE);
        total_respone_buffer_length+= RECEIVING_BUFFER_SIZE;
      }
      strncpy(total_respone_buffer + total_bytes_read, receive_buffer, bytes_read);
      total_bytes_read+= bytes_read;
      memset(receive_buffer + bytes_read, 0 ,RECEIVING_BUFFER_SIZE - bytes_read);
    }
    while(!end_of_http_request_twice_in_string(total_respone_buffer));

    *server_number = (*server_number + 1) % 3;
    return total_respone_buffer;
}

void send_back_response_to_client(char* response_from_server, const int* connection_fd){

    int total_bytes_sent = 0, total_bytes_to_send = strlen(response_from_server);

    while(total_bytes_sent < total_bytes_to_send){
      total_bytes_sent+=send(*connection_fd, response_from_server+total_bytes_sent,
          total_bytes_to_send - total_bytes_sent, 0);
    }
    shutdown(*connection_fd,SHUT_RDWR);
    free(response_from_server);
}