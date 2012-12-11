
#ifndef __828_DSM_OWNERSHIP__
#define __828_DSM_OWNERSHIP__
#include "dsm.h"

// Sentinel value for no owner (i.e. no owner is currently modifying)
#define NO_OWNER     (-1)

// Sentinel value for no response (i.e. no response is needed)
#define NO_RESPONSE  (-1)

// Page permission statuses by owner
#define MODIFYING    PROT_READ_WRITE
#define READING      PROT_READ
#define INVALIDATED  PROT_NONE

// Page permission statuses
#define MODIFIED     2
#define READABLE     1

struct PageStatus {
    int id;
    int status;
    int modifying_owner;
    int *status_by_owner;
};

int init_dsm_page_ownership (int number_of_owners,
        int number_of_pages, int my_id);

int get_read_copy (int page_number);
int get_write_copy (int page_number);

int quit_dsm_page_ownership();

int receive_message(int from, struct Message* msg);

// Error Codes
#define E_UNHANDLED_PAGE_STATUS      1
#define E_NOT_ENOUGH_MEMORY          2
#define E_INCORRECT_OWNER            3
#define E_INCONSISTENT_STATE         4
#define E_LOCK_INITIALIZATION_FAILED 5
#define E_OVERRAN_WAIT_COUNT         6
#define E_UNHANDLED_MESSAGE_TYPE     7

#endif // #ifndef __828_DSM_OWNERSHIP__
