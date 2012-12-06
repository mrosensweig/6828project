#include "dsm.h"

static sigsegv_dispatcher dispatcher;

void
dsm_init(void)
{
  sigsegv_init (&dispatcher);
  sigsegv_install_handler (&handler);
}

static int
dsm_area_handler (void *fault_address, void *user_arg)
{
  
}

static int
handler (void *fault_address, int serious)
{
  return sigsegv_dispatch (&dispatcher, fault_address);
}


