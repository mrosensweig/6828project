#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  
  dsm_start();
  printf("DSM SYSTEM STARTED: proc %d\n", proc_id);
  if (proc_id == 0) {
    int *p = (int *) (DSM_AREA_START + 0xb12);
    printf("Adding 10\n");
    *p = 10;
  } else if (proc_id == 1) {
    int *o = (int *) (DSM_AREA_START + 0xb12);
    printf("Adding 8\n");
    *o = 8;
  }else if (proc_id == 2) {
    int *q = (int *) (DSM_AREA_START + 0xb12);
    printf("Adding 6\n");
    *q = 6;
  }else if (proc_id == 3) {
    sleep(4);
    int *r = (int *) (DSM_AREA_START + 0xb12);
    printf("r should be 24: %d\n", *r); 
  }



  pthread_exit(0);
}
