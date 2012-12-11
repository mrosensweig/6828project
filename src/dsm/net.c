#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#include "fcntl.h"
#include "pthread.h"
#include "ownership.h"
#include "net.h"


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int proc_id = NCORES;
pid_t pids[NCORES];
int client_sockets[NCORES];

int
dsm_start() {
    spawn_processes();

    dsm_init(proc_id);
    start_server_thread();
    child_process();

    return 0;
}

int
spawn_processes() {
    int i;
    for(i=0; i<NCORES; i++) {
        int pid = fork();
        if(pid == 0){
        } else {
            pids[i] = pid;
            proc_id = i;
            break;
        }
    }

    if (i==NCORES) {
        exit(0);
    }

    return 0;
}

int
start_server_thread() {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&thread, &attr, &start_server, NULL);
}

int
child_process() {
        try_connecting_to_other_servers();
}

int
try_connecting_to_other_servers() {
    int i, j;

    for(i=0; i<NCORES; i++) {
        client_sockets[i] = -1;
    }

    int sockets_connected = 0;
    int attempts = (NCORES + 1) * 5;

    for(;;){
    //for(j=0; j<attempts; j++) {

        usleep(100000);
        for(i=0; i<NCORES; i++) {
            if(client_sockets[i] == -1) {
                printf("%d: Attempting to connect to %d\n", proc_id, i);
                int client_id = open_client_socket("localhost", 6000 + i);
                if(client_id < 0) {
                } else {
                    sockets_connected ++;
                    client_sockets[i] = client_id;
                }
            }
        }
        if(sockets_connected == NCORES) {
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
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
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
get_port() {
    return 6000 + proc_id;
}

static void*
start_server() {

    int port = get_port();

    // create a socket
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

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
    int accepted = NCORES;
    
    int sockets[NCORES];
    int i;
    for(i=0; i<NCORES; i++) {
        sockets[i] = -1;
    }

    while(accepted > 0) {
        int client_id = accept(socket_id, (struct sockaddr *) &client_address, &client_address_length);

        if(client_id < 0) {
            error("Error accepting connection.\n");
        }

        char buf[256];

        recv(client_id, buf, 256, 0);

        fcntl(client_id, F_SETFL, O_NONBLOCK);

        sockets[buf[0]] = client_id;

        accepted --;
    }

    printf("%d: accepted all connections.", proc_id);

    int exit = 0;

    char buf[sizeof(struct Message)];
    while(!exit) {

        for(i=0; i<NCORES; i++){
            if(i==proc_id) continue;

            int err = recv( sockets[i], buf, sizeof(struct Message), 0);
            if(err > 0) {
                // received some stuff
                buf[err] = '\0';
                if(buf[0] == EXIT) {
                    exit = 1;
                    break;
                }
                server_received(i, buf, err);
            } else {
            }
        }
    }

    close(socket_id);
    printf("%d: CLOSED IT BITCH\n", proc_id);

    return 0;
}

int
server_received(int i, char* buf, int len) {
    struct Message* msg = (struct Message *) buf;

    printf("%d: Message type: %c.\n", proc_id, msg->msg_type);
    return receive_message(i, msg);
}

int
send_message(int target, char type, char permissions, int page_number) {
    struct Message msg;

    msg.msg_type = type;
    msg.permissions = permissions;
    msg.page_number = page_number;

    send(client_sockets[target], (char *) &msg, sizeof(struct Message), 0);

    return 0;
}

int send_to(int target, struct Message* msg) {
    return send(client_sockets[target], (char *) msg, sizeof(struct Message), 0);
}
