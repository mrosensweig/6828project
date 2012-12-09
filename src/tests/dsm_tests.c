#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  dsm_init();
  int *a = (int *) DSM_AREA_START;

printf("before\n");
  int ab = *a;
printf("after: %p\n", &ab);
  *a = 17;
*a = 32;
  printf("a = %d\n", *a);
}
