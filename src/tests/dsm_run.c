#include "../dsm/dsm.h"

int die() {

      int i;
      for(i=0; i<NCORES; i++) {
  //        send_to(i, (struct Message *) "xlollll");
      }
}

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
    printf("Ten: %d\n", *o);
    die();
  } else {
  }


    
  pthread_exit(0);
}
