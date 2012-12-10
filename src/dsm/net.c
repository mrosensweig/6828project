#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#include "fcntl.h"

#define NUM_FORKS 4
int proc_id = NUM_FORKS;
pid_t pids[NUM_FORKS];
int client_sockets[NUM_FORKS];

int
main() {
    spawn_processes();
    start_server_thread();
    child_process();

    return 0;
}

int
spawn_processes() {
    int i;
    for(i=0; i<NUM_FORKS; i++) {
        int pid = fork();
        if(pid == 0){
        } else {
            pids[i] = pid;
            proc_id = i;
            break;
        }
    }

    if (i==NUM_FORKS) {
        exit(0);
    }

    return 0;
}

int
start_server_thread() {

    int pid = fork();
    if(pid == 0) {
        start_server(6000 + proc_id);
        exit(0);
    }
}

int
child_process() {
        try_connecting_to_other_servers();
}

int
try_connecting_to_other_servers() {
    int i, j;

    for(i=0; i<NUM_FORKS; i++) {
        client_sockets[i] = -1;
    }

    int sockets_connected = 0;
    int attempts = 5;

    for(j=0; j<attempts; j++) {

        usleep(1000000);
        for(i=0; i<NUM_FORKS; i++) {
            if(i != proc_id && client_sockets[i] == -1) {
                int client_id = open_client_socket("localhost", 6000 + i);
                if(client_id < 0) {
                } else {
                    printf("Succeeded!\n");
                    sockets_connected ++;
                    client_sockets[i] = client_id;
                }
            }
        }
        if(sockets_connected == NUM_FORKS-1) {
            printf("%d: Connected to all\n", proc_id);
            break;
        }
    }
}

int
open_client_socket(char *hostname, int port) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        return -1;

    char buf[20];
    buf[0] = proc_id;
    send(sockfd, buf, 20, 0);

    return sockfd;
}

int
start_server(int port) {

    // create a socket
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_id < 0) {
        error("Error opening server socket.\n");
    }

    struct sockaddr_in server_address;
    bzero( (char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if( bind(socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 ) {
        error("Error binding socket to address.\n");
    }

    // starting server
    printf("%d: Listening on port %d\n", proc_id, port);
    listen(socket_id, 5);

    // prepare to accept connection
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);
    int accepted = NUM_FORKS-1;
    
    int sockets[NUM_FORKS];

    while(accepted > 0) {
        int client_id = accept(socket_id, (struct sockaddr *) &client_address, &client_address_length);

        if(client_id < 0) {
            error("Error accepting connection.\n");
        }

        char buf[256];

        recv(client_id, buf, 256, 0);

        fcntl(client_id, F_SETFL, O_NONBLOCK);

        sockets[buf[0]] = client_id;

        printf("Accepted a connection from: %d\n", buf[0]);
        accepted --;
        close(client_id);
    }
    close(socket_id);

    return 0;
}
