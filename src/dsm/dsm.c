#include "dsm.h"

static sigsegv_dispatcher dispatcher;

void
dsm_init(void)
{
  sigsegv_init (&dispatcher);
  sigsegv_install_handler (&handler);

  void *p;
  unsigned long dsm_area;
  
  if ( (p = mmap( (void *) DSM_AREA_START, PGSIZE, PROT_NONE, 
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) < 0)
  {
    fprintf(stderr, "mmap failed\n");
    exit (2);
  }
  dsm_area = (unsigned long) p;

  sigsegv_register(&dispatcher, 
                   (void *)dsm_area,
                   PGSIZE * NPAGES,
                   &dsm_area_handler,
                   &dsm_area);
  
}

static int
dsm_area_handler (void *fault_address, void *user_arg)
{
  switch (permissions[get_pagenum(fault_address)]) {
    case PROT_NONE:
      //need to get accurrate value and switch to read
      printf("found PROT_NONE, changing to PROT_READ\n");
      if (set_permissions(fault_address, PGSIZE, PROT_READ) < 0) {
        fprintf(stderr, "Failure setting permissions at %p\n", fault_address);
        exit (2);
      }
      break;
    case PROT_READ:
      // attempted to write, so we need to make writeable and then invalidate
      printf("found PROT_READ, changing to PROT_READ_WRITE\n");
     
      if (set_permissions(fault_address, PGSIZE, PROT_READ_WRITE) < 0) {
        fprintf(stderr, "Failure setting write permissions at %p\n", fault_address);
        exit(2);
      }
    case PROT_READ_WRITE:
    default:
      /*
      not supporting executes yet, so if we fault here there is a problem
      */
      fprintf(stderr, "Attempted Execute at %p\n", fault_address);
      exit(2);
      break;
  }
}

static int
handler (void *fault_address, int serious)
{
  return sigsegv_dispatch (&dispatcher, fault_address);
}


