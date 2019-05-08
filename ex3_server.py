#!usr/bin/python2.7 -tt
import sys
from socket import *
import select
#import pdb


def main():
    RECEIVING_SIZE = 1024
    port = int(sys.argv[1])
    #pdb.set_trace()

    server_socket_server_one   = socket(AF_INET, SOCK_STREAM)
    server_socket_server_one.connect(('localhost', port))

    server_socket_server_two   = socket(AF_INET, SOCK_STREAM)
    server_socket_server_two.connect(('localhost', port))

    server_socket_server_three = socket(AF_INET, SOCK_STREAM)
    server_socket_server_three.connect(('localhost', port))

    socket_list = [server_socket_server_one, server_socket_server_two, server_socket_server_three]
    while True:
        ready_sockets,_,_ = select.select(socket_list, [], [])
        for ready_socket in ready_sockets:
            data_received = ready_socket.recv(RECEIVING_SIZE) # TODO: assumes the whole read is a complete request - maybe its not?
            response = process_data_received(data_received)
            ready_socket.send(response)





total_counter = 0
def process_data_received(data_received):
    get_line_of_address = data_received.split('\r\n')[0]
    if "counter" in get_line_of_address:
        total_counter += 1
        return """'HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length:""" + str(len(str(total_counter))) + """\r\n\r\n""" + str(total_counter) + """\r\n\r\n"""
    
    else:
        return """HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 113\r\n\r\n<html><head><title>Not Found</title></head><body<\r\nSorry, the object you requested was not found.\r\n</body></html>\r\n\r\n"""





if __name__ == "__main__":
    main()
    


