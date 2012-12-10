#include "../dsm/dsm.h"
#include <unistd.h>

int 
main(int argc, char **argv)
{
  int pid1;
  int pid2;
  int pipe12[2];
  int pipe21[2];

  if (pipe(pipe12) == -1 || pipe(pipe21) == -1) {
    fprintf(stderr, "pipe failed");
    exit(2);
  }
  
  if ((pid1 = fork()) == 0) {
    // init dsm for first process
    close(pipe12[0]);
    close(pipe21[1]);
    dsm_init(0, pipe21[0], pipe12[1]);
    int *p = (int *) (DSM_AREA_START + 0xb12);
    
    *p = 10;
    set_permissions((void *) DSM_AREA_START, PGSIZE, PROT_READ);
    printf("Setting p = %d in process %d\n", *p, getpid());
    *p = 10;
    close(pipe12[1]);
    close(pipe21[0]);
  } else {
    if ((pid2 = fork()) == 0) {
      // init dsm for second process
      close(pipe12[1]);
      close(pipe21[0]);
      dsm_init(1, pipe12[0], pipe21[1]);

      int *o = (int *) (DSM_AREA_START + 0xb12);
      
      set_permissions((void *)DSM_AREA_START, PGSIZE, PROT_NONE);
      sleep(2);
      /*
      while (*o != 10) {
        printf(".");
      }
      */
      printf("SUCCESS, p = %d in process %d\n", *o, getpid());
      close(pipe12[0]);
      close(pipe21[1]);
    }
  }
  wait();
  wait();
}
