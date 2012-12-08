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

extern void dsm_init(void);

static int dsm_area_handler (void *fault_address, void *user_arg);

static int handler (void *fault_address, int serious);

int set_permissions(void *addr, size_t len, int prot);
