#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  dsm_init(0);
  int *a = (int *) (DSM_AREA_START + 0x8000 + 0x34);

printf("before\n");
  int ab = *a;
printf("after: %p\n", &ab);
  *a = 17;
*a = 32;
  printf("a = %d\n", *a);
}
