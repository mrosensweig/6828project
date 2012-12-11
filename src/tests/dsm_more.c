#include "../dsm/dsm.h"

int 
main(int argc, char **argv)
{
  
  dsm_start();
  printf("DSM SYSTEM STARTED: proc %d\n", proc_id);
  if (proc_id == 0) {
    int *p = (int *) (DSM_AREA_START + 0xb12);
    printf("Setting p = 10\n");
    *p = 10;
  } else if (proc_id == 1) {
    sleep(2);
    int *o = (int *) (DSM_AREA_START + 0xb12);
    int *oo = (int *) (DSM_AREA_START + 0x1b12);
    printf("Ten: %d\n", *o);
    printf("Fourteen: %d\n", *oo);
  } else if (proc_id == 2) {
    int *q = (int *) (DSM_AREA_START + 0x1b12);
    printf("Setting q = 14\n");
    *q = 14;
  }

/*
  int i;
  for(i=0; i<NCORES; i++) {
      send_to(i, (struct Message *) "xlollll");
    }
  */  
  pthread_exit(0);
}
