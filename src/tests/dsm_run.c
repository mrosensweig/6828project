#include "../dsm/dsm.h"
#include <unistd.h>

int 
main(int argc, char **argv)
{
  int pid1;
  int pid2;
  int pipe12[2];
  int pipe21[2];

  /* Create two pipes to communicate between the processes
     that are sharing the memory locations */
  if (pipe(pipe12) == -1 || pipe(pipe21) == -1) {
    fprintf(stderr, "pipe failed");
    exit(2);
  }
  
  if ((pid1 = fork()) == 0) {
    // close pipes we are not using
    close(pipe12[0]);
    close(pipe21[1]);

    // init dsm for first process
    dsm_init(0, pipe21[0], pipe12[1]);

    //pick a memory location in the dsm range to use
    //as out example value
    int *p = (int *) (DSM_AREA_START + 0xb12);
    *p = 10;

    /* map p read only in our address space, so any attempted write
       to this value will cause a fault.  Then print that we are 
       altering the value of p */
    sleep(1);
    set_permissions((void *) DSM_AREA_START, PGSIZE, PROT_READ);
    printf("Setting p = %d in process %d\n", *p, getpid());
    *p = 10;

    // close all pipes, so wait will return
    close(pipe12[1]);
    close(pipe21[0]);
  } else {
    if ((pid2 = fork()) == 0) {
      // close unused pipe ends
      close(pipe12[1]);
      close(pipe21[0]);

      // init dsm for second process
      dsm_init(1, pipe12[0], pipe21[1]);

      /* use same memory location for the value here, to see if we can
         changes in one process affecting memory in the other one,
         and print out what we are doing */
      int *o = (int *) (DSM_AREA_START + 0xb12);
      printf("Reading p = %d in process %d\n", *o, getpid());
      set_permissions((void *)DSM_AREA_START, PGSIZE, PROT_NONE);
      sleep(2);
      /*
      while (*o != 10) {
        printf(".");
      }
      */
      // read new value and close old pipes
      printf("SUCCESS, p = %d in process %d\n", *o, getpid());
      close(pipe12[0]);
      close(pipe21[1]);
    }
  }
  // wait for both children to be done.
  wait();
  wait();
}
