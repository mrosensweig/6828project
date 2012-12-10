#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

int
main() {
    printf("Hello world\n");

    // create a socket
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_id < 0) {
        error("Error opening server socket.\n");
    }

    // generate the port and server info for bind()
    int port = 6000;

    struct sockaddr_in server_address;
    bzero( (char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if( bind(socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 ) {
        error("Error binding socket to address.\n");
    }

    // starting server
    printf("Listening...\n");
    listen(socket_id, 5);

    // prepare to accept connection
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);
    int client_id = accept(socket_id, (struct sockaddr *) &client_address, &client_address_length);

    if(client_id < 0) {
        error("Error accepting connection.\n");
    }

    printf("Accepted a connection.  Quitting.\n");
    close(client_id);
    close(socket_id);

    return 0;
}
