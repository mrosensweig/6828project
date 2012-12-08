#include "dsm.h"

int permissions[NPAGES];

int set_permissions(void *addr, size_t len, int prot)
{
  int dsm_page_num;
  unsigned long a = (unsigned long) addr;

  dsm_page_num = (a - DSM_AREA_START) / PGSIZE;

  if (dsm_page_num < 0 || dsm_page_num > NPAGES - 1) {
    return -1;
  }

  permissions[dsm_page_num] = prot;
   
  return mprotect(addr, len, prot);
}
