#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  dsm_init();
  void *a = (void *) DSM_AREA_START;

  unsigned long page = (unsigned long) a;
  int z = *(volatile int *) (page);
}
