#ifndef __6828_dsm_h__
#define __6828_dsm_h__

#include "sigsegv.h"
#include "sys/mman.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if HAVE_SIGSEGV_RECOVERY
#define PROT_READ_WRITE (PROT_READ|PROT_WRITE)
#define PGSIZE 0x1000
#define DSM_AREA_START 0x40000000
#define NPAGES 10
#define NCORES 4
#endif

extern int proc_id;

struct Message {
    char msg_type;
    char permissions;
    int page_number;
    int index;
    int is_response;
    char page[PGSIZE];
};

//Permissions of each dsm page on this machine
extern int permissions[NPAGES];

// initialize sockets and dsm stuff
extern int dsm_start(void);

//initialize dsm functions and handlers
extern void dsm_init(int my_id);

//handler for dsm region
static int dsm_area_handler (void *fault_address, void *user_arg);

//global sigsegv handler
static int handler (void *fault_address, int serious);

//wrapper around mprotect to update permissions array
int set_permissions(void *addr, size_t len, int prot);

//get dsm page number to index into permissions array
int get_pagenum(void *addr);

//get dsm address from index into permissions array
void *get_pageaddr(int pagenum);

//align a void pointer to a page boundary
void *page_align(void *addr);

#endif
