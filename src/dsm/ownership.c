#include "ownership.h"

static struct InfoSuite *ownershipInfo;

int
init_dsm_page_ownership (int number_of_owners,
        int number_of_pages, int my_id) {
    ownershipInfo = (struct InfoSuite *) malloc(sizeof(struct InfoSuite));
    if (ownershipInfo == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    ownershipInfo->nowners = number_of_owners;
    ownershipInfo->npages = number_of_pages;
    ownershipInfo->thisid = my_id;
    int how_many_pages_i_own = (number_of_pages / number_of_owners) + 1;
    ownershipInfo->page_statuses = (struct PageStatus *)
            malloc(how_many_pages_i_own * sizeof(struct PageStatus));
    if (ownershipInfo == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    int i;
    for (i = 0; i < how_many_pages_i_own; i++) {
        struct PageStatus *page_status = &ownershipInfo->page_statuses[i];
        page_status->id = i * ownershipInfo->nowners + ownershipInfo->thisid;
        page_status->status = READABLE;
        page_status->modifying_owner = NO_OWNER;
        page_status->status_by_owner = (int *)
                malloc(ownershipInfo->nowners * sizeof(int));
        if (page_status->status_by_owner == NULL) {
            return -E_NOT_ENOUGH_MEMORY;
        }
        int j;
        for (j = 0; j < ownershipInfo->nowners; j++) {
            page_status->status_by_owner[j] = READING;
        }
    }
    return 0;
}

static int
get_owner (int page_number) {
    return page_number % ownershipInfo->nowners;
}

int
get_read_copy (int page_number) {
    int owner_id = get_owner(page_number);
    if (owner_id == ownershipInfo->thisid) {
        // Ask myself for read-ownership
        return give_read_copy(ownershipInfo->thisid, page_number);
    } else {
        // Ask for read-ownership over network
    }
}

int
get_write_copy (int page_number) {
    int owner_id = get_owner(page_number);
    if (owner_id == ownershipInfo->thisid) {
        return give_write_copy(ownershipInfo->thisid, page_number);
    } else {
        // Ask for write-ownership over network
    }
}

static struct PageStatus *
get_page_status (int page_number) {
    return &ownershipInfo->page_statuses[page_number / ownershipInfo->nowners];
}

int
give_read_copy (int requester_id, int page_number) {
    int owner_id = get_owner(page_number);
    if (owner_id != ownershipInfo->thisid) {
        return -E_INCORRECT_OWNER;
    }
    struct PageStatus *page_status = get_page_status(page_number);
    switch (page_status->status) {
        case READABLE: {
            if (requester_id == ownershipInfo->thisid) {
                // Status should not be readable without local copy having
                //   read permissions; with these permissions, a page fault
                //   should not be thrown and this should never be called
                return -E_INCONSISTENT_STATE;
            }
            // Transfer page over network to requester_id and tell it it has
            //   PROT_READ access
            page_status->status_by_owner[requester_id] = READABLE;
            return 0;
        }
        case MODIFIED: {
            int modifier = page_status->modifying_owner;
            if (modifier == requester_id) {
                // The modifier should have read-write permissions, in which
                //   case a page not fault should not be thrown and this
                //   should never be called.
                return -E_INCONSISTENT_STATE;
            }
            if (modifier != ownershipInfo->thisid) {
                // Tell modifier that it is READING
                // Get modified page from modifier
                // Map modified page locally
                // Set local page to PROT_READ
                page_status->status_by_owner[ownershipInfo->thisid] = READING;
            }
            page_status->status_by_owner[modifier] = READING;
            page_status->modifying_owner = NO_OWNER;
            page_status->status = READABLE;
            if (requester_id == ownershipInfo->thisid) {
                page_status->status_by_owner[requester_id] = READING;
                // Transfer page over network to requester_id and tell it it
                //   has PROT_READ access
            }
            return 0;
        }
        default:
            printf("Unhandled page status in give_read_copy.\n");
            exit(-E_UNHANDLED_PAGE_STATUS);
    }
}

int
give_write_copy (int requester_id, int page_number) {
    int owner_id = get_owner(page_number);
    if (owner_id != ownershipInfo->thisid) {
        return -E_INCORRECT_OWNER;
    }
    struct PageStatus *page_status = get_page_status(page_number);
    switch (page_status->status) {
        case READABLE: {
            int i;
            for (i = 0; i < ownershipInfo->nowners; i++) {
                if (i == requester_id) {
                    continue;
                }
                if (i == ownershipInfo->thisid) {
                    if (page_status->status_by_owner[i] == READING) {
                        // Set local page to PROT_NONE
                        page_status->status_by_owner[i] = INVALIDATED;
                        continue;
                    } else {
                        // This shouldn't happen - the local copy should become
                        //   readable as soon the page status becomes READABLE
                        return -E_INCONSISTENT_STATE;
                    }
                }
                if (page_status->status_by_owner[i] == READING) {
                    // Tell i that it is INVALIDATED
                    page_status->status_by_owner[i] = INVALIDATED;
                }
            }
            page_status->status_by_owner[requester_id] = MODIFYING;
            page_status->modifying_owner = requester_id;
            page_status->status = MODIFIED;
            if (requester_id == ownershipInfo->thisid) {
                // Set local page to PROT_READ_WRITE
            } else {
                // Transfer page over network to requester_id and tell it it has
                //   PROT_READ_WRITE access
            }
            return 0;
        }
        case MODIFIED: {
            int modifier = page_status->modifying_owner;
            if (modifier == requester_id) {
                // The modifier should have read-write permissions, in which
                //   case a page not fault should not be thrown and this
                //   should never be called.
                return -E_INCONSISTENT_STATE;
            }
            if (modifier == ownershipInfo->thisid) {
                // Set local page to PROT_NONE
            } else {
                // Tell modifier that it is INVALIDATED
                // Get modified page from modifier
                // Map modified page locally
            }
            page_status->status_by_owner[modifier] = INVALIDATED;
            page_status->status_by_owner[requester_id] = MODIFYING;
            page_status->modifying_owner = requester_id;
            if (requester_id == ownershipInfo->thisid) {
                // Set local page to PROT_READ_WRITE
            } else {
                // Transfer page over network to requester_id and tell it it has
                //   PROT_READ_WRITE access
            }
            return 0;
        }
        default: {
            printf("Unhandled page status in give_read_copy.\n");
            exit(-E_UNHANDLED_PAGE_STATUS);
        }
    }
}

int
quit_dsm_page_ownership () {
    int i;
    for (i = 0; i < ownershipInfo->nowners; i++) {
        free(ownershipInfo->page_statuses[i].status_by_owner);
        ownershipInfo->page_statuses[i].status_by_owner = NULL;
    }
    free(ownershipInfo->page_statuses);
    ownershipInfo->page_statuses = NULL;
    free(ownershipInfo);
    ownershipInfo = NULL;
    return 0;
}

int
receive_message(int from, struct Message* msg) {
    // YO PUT YOU SHIT HERE
    return 0;
}
