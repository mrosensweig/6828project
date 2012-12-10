#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  int pid1;
  int pid2;
  
  if ((pid1 = fork()) == 0) {
    // init dsm for first process
    dsm_init(0);
    int *p = (int *) (DSM_AREA_START + 0xb12);
    
    *p = 10;
  } else {
    if ((pid2 = fork()) == 0) {
      // init dsm for second process
      dsm_init(1);
      int *o = (int *) (DSM_AREA_START + 0xb12);

      while (*o != 10) {
        printf(".");
      }
      printf("SUCCESS");
    }
  }
}
