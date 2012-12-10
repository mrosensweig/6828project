#ifndef __828_DSM_NET__
#define __828_DSM_NET__

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"
#include "dsm.h"
#include "ownership.h"

#define NUM_FORKS 4

int spawn_processes();
int start_server_thread();
int child_process();
int try_connecting_to_other_servers();
static void* start_server();

int send_message(int target, char type, char permissions, int page_number);
int send_to(int target, struct Message* msg);

#endif
