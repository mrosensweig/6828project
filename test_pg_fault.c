#include "sigsegv.h"
#include "sys/mman.h"
#include <stdio.h>
#include <stdlib.h>

#if HAVE_SIGSEGV_RECOVERY

static int zero_fd;

int
handler(void *fault_address, int serious)
{
  printf("handler called");

  
  return 0;
}

int
main()
{
  void *a;

  a = mmap( (void *) 0x12340000, 0x4000, PROT_WRITE, MAP_SHARED, zero_fd, 0);

  mprotect( a, 0x400, PROT_READ );
  sigsegv_install_handler(&handler);

  *(volatile int *) (a + 0x234) = 17;

  return 1;
}

#else

int
main()
{
  printf("fail\n");
}

#endif
