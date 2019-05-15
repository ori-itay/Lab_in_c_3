#!usr/bin/python2.7 -tt
import sys
from socket import *
import select

def main():
    RECEIVING_SIZE = 1024
    INIT_COUNT = 0
    port = int(sys.argv[1])
    servers_request_counter_list = [INIT_COUNT, INIT_COUNT, INIT_COUNT]

    server_socket_server_one   = socket()
    server_socket_server_one.connect(('localhost', port))

    server_socket_server_two   = socket()
    server_socket_server_two.connect(('localhost', port))

    server_socket_server_three = socket()
    server_socket_server_three.connect(('localhost', port))

    socket_list = [server_socket_server_one, server_socket_server_two, server_socket_server_three]
    while True:
        ready_sockets,_,_ = select.select(socket_list, [], [])
        for ready_socket in ready_sockets:
            data_received = receive_from_ready_socket(ready_socket, RECEIVING_SIZE)
            response = process_data_received(data_received, servers_request_counter_list, \
                                             socket_list.index(ready_socket))
            ready_socket.sendall(response)

def receive_from_ready_socket(ready_socket, RECEIVING_SIZE):
    END_OF_HTTP_REQUEST = "\r\n\r\n"
    data_received = ""
    while(END_OF_HTTP_REQUEST not in data_received):
        data_received+= ready_socket.recv(RECEIVING_SIZE)

    return data_received

def process_data_received(data_received, servers_request_counter_list, current_socket_index):
    get_line_of_address = data_received.split('\r\n')[0]
    if "counter " in get_line_of_address:
        servers_request_counter_list[current_socket_index] = servers_request_counter_list[current_socket_index] + 1
        http_response = """HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: """ + \
                        str(len(str(servers_request_counter_list[current_socket_index]))) + """\r\n\r\n""" + \
                        str(servers_request_counter_list[current_socket_index]) + """\r\n\r\n"""
    else:
        http_response = """HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: \ 113\r\n\r\n
        <html><head><title>Not Found</title></head><body>\r\n
        Sorry, the object you requested was not found.\r\n</body></html>\r\n\r\n"""

    return http_response

if __name__ == "__main__":
    main()