#include "sigsegv.h"
#include "sys/mman.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if HAVE_SIGSEGV_RECOVERY
#define PROT_READ_WRITE (PROT_READ|PROT_WRITE)
#define PGSIZE 0x4000
#define DSM_AREA_START 0x40000000
#define NPAGES 1
#endif

//Permissions of each dsm page on this machine
extern int permissions[NPAGES];

//initialize dsm functions and handlers
extern void dsm_init(void);

//handler for dsm region
static int dsm_area_handler (void *fault_address, void *user_arg);

//global sigsegv handler
static int handler (void *fault_address, int serious);

//wrapper around mprotect to update permissions array
int set_permissions(void *addr, size_t len, int prot);

//get dsm page number to index into permissions array
int get_pagenum(void *addr);
