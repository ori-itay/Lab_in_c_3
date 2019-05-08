#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <sys/socket.h> // check that all are neccessary
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXIMUM_PORT 64000
#define STARTING_RANGE_PORT 1024
#define RECEIVING_BUFFER_SIZE 1024

void handle_connections(int *socket);
void bind_random_port(int* socket ,struct sockaddr_in* server, struct sockaddr_in* http_server);

int main(){
    int listen_socket = -1, opt = 1;
    struct sockaddr_in server ={0}, http_server = {0};
    srand(time(0));
    

    listen_socket = socket(AF_INET, SOCK_STREAM, 0); // check for failure?
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); // check for failure?

    bind_random_port(&listen_socket, &server, &http_server);
    listen(listen_socket, 10); // check for failure? change from 10 !
    handle_connections(&listen_socket); // dont care about peer?

     






    return EXIT_SUCCESS;
}


void handle_connections(int *socket){
    int connection_fd = -1, bytes_read, times_seen_end_of_http_request = 0;
    char buffer[RECEIVING_BUFFER_SIZE] ={0};
    while (true){
        connection_fd = accept(*socket, NULL, NULL);  // check for failure?

        bytes_read = recv(connection_fd, buffer, RECEIVING_BUFFER_SIZE, 0);
        if (bytes_read == -1){// remove ?
            printf("bytes read returned -1\n"); // remove ?
            break;
        }
        printf("%s", buffer);
        if (bytes_read == 0){
            printf("bytes read returned 0\n");

            break;
        }
    }
}


void bind_random_port(int* socket ,struct sockaddr_in* server, struct sockaddr_in* http_server){
    int random_port;
    (*server).sin_family = AF_INET;
    (*server).sin_addr.s_addr =  INADDR_ANY;

    while (true){
        random_port = rand() % (MAXIMUM_PORT+1);
        if (random_port < STARTING_RANGE_PORT){
            random_port += STARTING_RANGE_PORT;
        }
        // (*server).sin_port = htons(random_port);
        // if (bind(*socket, (struct sockaddr*)server, sizeof(*server)) < 0 ){
        //     continue;
        // }



        (*server).sin_port = htons(63998); // remove !!
        if (bind(*socket, (struct sockaddr*)server, sizeof(*server)) < 0 ){ // remove !!
            printf("Binding to port 80 failed!\n");
            exit(EXIT_FAILURE);
        }
        else {
            break;
        }
    }
}