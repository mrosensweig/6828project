#include "sigsegv.h"
#include "sys/mman.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if HAVE_SIGSEGV_RECOVERY
#define PROT_READ_WRITE (PROT_READ|PROT_WRITE)
unsigned long page;

int
handler(void *fault_address, int serious)
{
  printf("handler called\n");

  if ( mprotect ( (void *) page, 0x4000, PROT_READ_WRITE) == 0 )
    return 1; 
  
  return 0;
}

int
main()
{
  void *a;
  int e;

  if ((a = mmap( (void *) 0x12340000, 0x4000, 
                      PROT_READ_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) < 0) 
  {
    fprintf(stderr, "memmap failed\n");
    exit (2);
  }
  page = (unsigned long) a;
  printf("a = %p\n", a);

  // make it read only
  if ((e = mprotect( (void *) page, 0x4000, PROT_READ )) < 0)
  {
    fprintf(stderr, "mprotect failed: %d\n", errno);
    exit(3);
  }

  /* Test whether it's possible to make it read-write after it was read-only.
     This is not possible on Cygwin.  */
  if (mprotect ((void *) page, 0x4000, PROT_READ_WRITE) < 0
      || mprotect ((void *) page, 0x4000, PROT_READ) < 0)
    {
      fprintf (stderr, "mprotect failed.\n");
      exit (2);
    }


  sigsegv_install_handler(&handler);

  *(volatile int *) (page + 0x234) = 17;
  *(volatile int *) (page + 0x234) = 12;

  return 0;
}

#else

int
main()
{
  printf("fail\n");
  return -1;
}

#endif
