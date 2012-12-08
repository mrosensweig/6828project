#include "dsm.h"

static sigsegv_dispatcher dispatcher;

void
dsm_init(void)
{
  sigsegv_init (&dispatcher);
  sigsegv_install_handler (&handler);

  void *p;
  unsigned long dsm_area;
  
  if ( (p = mmap( (void *) DSM_AREA_START, PGSIZE, PROT_READ, 
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) < 0)
  {
    fprintf(stderr, "mmap failed\n");
    exit (2);
  }
  dsm_area = (unsigned long) p;

  sigsegv_register(&dispatcher,(void *)dsm_area,PGSIZE,&dsm_area_handler,&dsm_area);
  
}

static int
dsm_area_handler (void *fault_address, void *user_arg)
{
  printf("area handler called\n");
}

static int
handler (void *fault_address, int serious)
{
  return sigsegv_dispatch (&dispatcher, fault_address);
}


