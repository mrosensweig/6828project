#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  dsm_init();
  int *a = (int *) DSM_AREA_START;

  set_permissions((void *)DSM_AREA_START, PGSIZE, PROT_READ);
  
printf("before\n");
  int ab = *a;
printf("after: %p\n", &ab);
  *a = 17;

  printf("a = %d\n", *a);
}
