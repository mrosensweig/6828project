#include "dsm.h"

int permissions[NPAGES];

int set_permissions(void *addr, size_t len, int prot)
{
  int dsm_page_num;

  dsm_page_num = get_pagenum(addr);

  if (dsm_page_num < 0 || dsm_page_num > NPAGES - 1) {
    fprintf(stderr, "Error: out of dsm range");
    return -1;
  }

  permissions[dsm_page_num] = prot;
   
  return mprotect(addr, len, prot);
}


int
get_pagenum(void *addr)
{
  unsigned long a = (unsigned long) addr;

  return (a - DSM_AREA_START) / PGSIZE;
}

void *
get_pageaddr(int pagenum)
{
    return (void *) (pagenum * PGSIZE + (unsigned long) DSM_AREA_START);
}

void *
page_align(void *addr)
{
  unsigned long page = (unsigned long) addr;
  page %= PGSIZE;
  return (void *) ((unsigned long) addr - page);
}

