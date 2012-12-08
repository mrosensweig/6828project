#include "dsm.h"

int permissions[NPAGES];

int set_permissions(void *addr, size_t len, int prot)
{
  int dsm_page_num;
  
  return mprotect(addr, len, prot);
}
