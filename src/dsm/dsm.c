#include "dsm.h"
#include "ownership.h"

static sigsegv_dispatcher dispatcher;

void
dsm_init(int my_id)
{
  sigsegv_init (&dispatcher);
  sigsegv_install_handler (&handler);

  void *p;
  unsigned long dsm_area;
  
  if ( (p = mmap( (void *) DSM_AREA_START, PGSIZE * NPAGES, PROT_READ_WRITE, 
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) < 0)
  {
    fprintf(stderr, "mmap failed\n");
    exit (2);
  }
  dsm_area = (unsigned long) p;

  if (mprotect((void *) dsm_area, PGSIZE * NPAGES, PROT_READ) < 0) {
    fprintf(stderr, "mrpotect failed at %p\n", (void *)dsm_area);
    exit(2);
  }
  int i;
  for (i = 0; i < NPAGES; i++)
    permissions[i] = PROT_READ;

  sigsegv_register(&dispatcher, 
                   (void *)dsm_area,
                   PGSIZE * NPAGES,
                   &dsm_area_handler,
                   &dsm_area);
  
  if (init_dsm_page_ownership(NCORES, NPAGES, my_id) < 0) {
    fprintf(stderr, "init page ownership failed");
    exit(2);
  }
}

static int
dsm_area_handler (void *fault_address, void *user_arg)
{
  void *aligned_addr = page_align(fault_address);
  int perms = permissions[get_pagenum(fault_address)];
    if ((perms | PROT_NONE) == PROT_NONE) {
      //need to get accurrate value and switch to read
      printf("found PROT_NONE, changing to PROT_READ\n");
      
      if (get_read_copy(get_pagenum(fault_address)) < 0) {
        fprintf(stderr, "error getting read only copy");
        exit(2);
      }
      
      if (set_permissions(aligned_addr, PGSIZE, PROT_READ) < 0) {
        fprintf(stderr, "Failure setting permissions at %p\n", fault_address);
        exit (2);
      }
      return 1;
    } else if ((perms | PROT_READ) == PROT_READ) {
      // attempted to write, so we need to make writeable and then invalidate
      printf("found PROT_READ, changing to PROT_READ_WRITE\n");
      
      if (get_write_copy(get_pagenum(fault_address)) < 0){
        fprintf(stderr, "error getting writeable page");
        exit(2);
      }

      if (set_permissions(aligned_addr, PGSIZE, PROT_READ_WRITE) < 0) {
        fprintf(stderr, "Failure setting write permissions at %p\n", fault_address);
        exit(2);
      }
      return 2;
    } else {
      /*
      not supporting executes yet, so if we fault here there is a problem
      */
      fprintf(stderr, "Attempted Execute at %p\n", fault_address);
      exit(2);
      return 3;
    }
  return 4;
}

static int
handler (void *fault_address, int serious)
{
  return sigsegv_dispatch (&dispatcher, fault_address);
}


