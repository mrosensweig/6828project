#include "ownership.h"
#include "net.h"
#include <pthread.h>

static int npages;
static int nowners;
static int thisid;
static struct PageStatus *page_statuses;
static pthread_mutex_t *locks;
static pthread_cond_t *waits;
static int wait_indices_index;
static int *wait_indices;
static pthread_mutex_t *wait_indices_index_lock;

static int
get_number_of_locks () {
    return npages;
}

static int
get_number_of_waits () {
    return 2 * npages;
}

static int 
init_page_statuses () {
    int how_many_pages_i_own = (npages / nowners) + 1;
    page_statuses = (struct PageStatus *)
            malloc(how_many_pages_i_own * sizeof(struct PageStatus));
    if (page_statuses == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    int i;
    for (i = 0; i < how_many_pages_i_own; i++) {
        struct PageStatus *page_status = &page_statuses[i];
        page_status->id = i * nowners + thisid;
        page_status->status = READABLE;
        page_status->modifying_owner = NO_OWNER;
        page_status->status_by_owner = (int *)
                malloc(nowners * sizeof(int));
        if (page_status->status_by_owner == NULL) {
            return -E_NOT_ENOUGH_MEMORY;
        }
        int j;
        for (j = 0; j < nowners; j++) {
            page_status->status_by_owner[j] = READING;
        }
    }
    return 0;
}

static int
init_locks () {
    int i;
    int number_of_locks = get_number_of_locks();
    locks = (pthread_mutex_t *)
            malloc(number_of_locks * sizeof(pthread_mutex_t));
    if (locks == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    for (i = 0; i < number_of_locks; i++) {
        if (pthread_mutex_init(&locks[i], NULL) != 0) {
            return -E_LOCK_INITIALIZATION_FAILED;
        }
    }
    wait_indices_index = 0;
    wait_indices_index_lock = (pthread_mutex_t *)
            malloc(sizeof(pthread_mutex_t));
    if (wait_indices_index_lock == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    if (pthread_mutex_init(wait_indices_index_lock, NULL) != 0) {
        return -E_LOCK_INITIALIZATION_FAILED;
    }
    return 0;
}

static int
init_waits() {
    int i;
    int number_of_waits = get_number_of_waits();
    waits = (pthread_cond_t *)
            malloc(number_of_waits * sizeof(pthread_cond_t));
    if (waits == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    for (i = 0; i < number_of_waits; i++) {
        if (pthread_cond_init(&waits[i], NULL) != 0) {
            return -E_LOCK_INITIALIZATION_FAILED;
        }
    }
    wait_indices = (int *)
            malloc(number_of_waits * sizeof(int));
    if (wait_indices == NULL) {
        return -E_NOT_ENOUGH_MEMORY;
    }
    for (i = 0; i < number_of_waits; i++) {
        wait_indices[i] = i;
    }
    return 0;
}

int
init_dsm_page_ownership (int number_of_owners, int number_of_pages, int my_id) {
    int r;
    nowners = number_of_owners;
    npages = number_of_pages;
    thisid = my_id;
    if ((r = init_page_statuses()) < 0) {
        return r;
    }
    if ((r = init_locks()) < 0) {
        return r;
    }
    if ((r = init_waits()) < 0) {
        return r;
    }
    return 0;
}


static int
get_wait_index () {
    pthread_mutex_lock(wait_indices_index_lock);
    int result = wait_indices[wait_indices_index];
    if (wait_indices_index >= get_number_of_waits()) {
        return -E_OVERRAN_WAIT_COUNT;
    }
    wait_indices_index += 1;
    pthread_mutex_unlock(wait_indices_index_lock);
    return result;
}

static void
return_wait_index (int wait_index) {
    pthread_mutex_lock(wait_indices_index_lock);
    wait_indices_index -= 1;
    wait_indices[wait_indices_index] = wait_index;
    pthread_mutex_unlock(wait_indices_index_lock);
}

static struct Message
create_message (int type, int perms, int pagenum) {
    struct Message m;
    m.msg_type = type;
    m.permissions = perms;
    m.page_number = pagenum;
    m.is_response = NO_RESPONSE;
}

static int
send_and_wait_for_response (int target, struct Message *m) {
    int result;
    int wait_index;
    if ((wait_index = get_wait_index()) < 0) {
        // Error
        return wait_index;
    }
    m->index = wait_index;
    // Send message over network
    if ((result = send_to(target, m)) < 0) {
        return result;
    }
    // wait for response
    pthread_cond_wait(&waits[wait_index], &locks[m->page_number]);
    return_wait_index(wait_index);
    return 0;
}

static int
get_owner (int page_number) {
    return page_number % nowners;
}

static struct PageStatus *
get_page_status (int page_number) {
    return &page_statuses[page_number / nowners];
}

static int
give_read_copy (int requester_id, int request_id, int page_number) {
    int result;
    pthread_mutex_lock(&locks[page_number]);
    int owner_id = get_owner(page_number);
    if (owner_id != thisid) {
        pthread_mutex_unlock(&locks[page_number]);
        return -E_INCORRECT_OWNER;
    }
    struct PageStatus *page_status = get_page_status(page_number);
    switch (page_status->status) {
        case READABLE: {
            if (requester_id == thisid) {
                // Status should not be readable without local copy having
                //   read permissions; with these permissions, a page fault
                //   should not be thrown and this should never be called
                result = -E_INCONSISTENT_STATE;
                break;
            }
            // Transfer page over network to owner_id and tell it it
            //   has PROT_READ access
            struct Message m = create_message(SEND_PAGE, READING, page_number);
            memcpy((void *) m.page, (void *) get_pageaddr(page_number), PGSIZE);
            m.is_response = 1;
            if (request_id == NO_RESPONSE) {
                result = E_INCONSISTENT_STATE;
                break;
            }
            m.index = request_id;
            if ((result = send_to(owner_id, &m)) < 0) break;
            page_status->status_by_owner[requester_id] = READABLE;
            result = 0;
            break;
        }
        case MODIFIED: {
            int modifier = page_status->modifying_owner;
            if (modifier == requester_id) {
                // The modifier should have read-write permissions, in which
                //   case a page not fault should not be thrown and this
                //   should never be called.
                result = -E_INCONSISTENT_STATE;
                break;
            }
            if (modifier != thisid) {
                // Request page from modifier and tell it to set its
                //   permissions to PROT_READ
                struct Message m =
                    create_message(REQUEST_PAGE, READING, page_number);
                // Upon receiving the message we should map the page locally
                if ((result = send_and_wait_for_response(modifier, &m)) < 0) {
                    break;
                }
                // Local page should already be set PROT_READ
                page_status->status_by_owner[thisid] = READING;
            }
            page_status->status_by_owner[modifier] = READING;
            page_status->modifying_owner = NO_OWNER;
            page_status->status = READABLE;
            if (requester_id != thisid) {
                // Transfer page over network to requester_id and tell it it
                //   has PROT_READ access
                struct Message m =
                        create_message(SEND_PAGE, READING, page_number);
                memcpy((void *) m.page, (void *) get_pageaddr(page_number),
                        PGSIZE);
                m.is_response = 1;
                if (request_id == NO_RESPONSE) {
                    result = E_INCONSISTENT_STATE;
                    break;
                }
                m.index = request_id;
                result = send_to(requester_id, &m);
                if (result < 0) break;
                page_status->status_by_owner[requester_id] = READING;
            }
            result = 0;
            break;
        }
        default:
            printf("Unhandled page status in give_read_copy.\n");
            exit(-E_UNHANDLED_PAGE_STATUS);
    }
    pthread_mutex_unlock(&locks[page_number]);
    return result;
}

static int
give_write_copy (int requester_id, int request_id, int page_number) {
    int result;
    printf("%d: pre-lock\n", thisid);
    pthread_mutex_lock(&locks[page_number]);
    printf("%d: post-lock\n", thisid);
    int owner_id = get_owner(page_number);
    if (owner_id != thisid) {
        printf("%d: wtf\n", thisid);
        pthread_mutex_unlock(&locks[page_number]);
        return -E_INCORRECT_OWNER;
    }
    struct PageStatus *page_status = get_page_status(page_number);
    printf("%d: page_status->status: %d\n", thisid, page_status->status);
    switch (page_status->status) {
        case READABLE: {
            int i;
            int had_read_only;
            for (i = 0; i < nowners; i++) {
                if (i == requester_id) {
                    had_read_only = (page_status->status_by_owner[i] == READING);
                    continue;
                }
                if (i == thisid) {
                    if (page_status->status_by_owner[i] == READING) {
                        // Set local page to PROT_NONE at end of function
                        continue;
                    } else {
                        // This shouldn't happen - the local copy should become
                        //   readable as soon the page status becomes READABLE
                        result = -E_INCONSISTENT_STATE;
                        break;
                    }
                }
                if (page_status->status_by_owner[i] == READING) {
                    // Tell i that it is INVALIDATED
                    printf("%d: Telling %d that it is INVALIDATED\n", thisid, i);
                    struct Message m = create_message(SET_PERMISSION,
                            INVALIDATED, page_number);
                    result = send_to(i, &m);
                    if (result < 0) break;
                    page_status->status_by_owner[i] = INVALIDATED;
                }
            }
            page_status->status_by_owner[requester_id] = MODIFYING;
            page_status->modifying_owner = requester_id;
            page_status->status = MODIFIED;
            if (requester_id == thisid) {
                // Set local page to PROT_READ_WRITE
                printf("%d: Setting local page %d to PROT_READ_WRITE\n", thisid,
                        page_number);
                result = set_permissions((void *) get_pageaddr(page_number),
                        PGSIZE, PROT_READ_WRITE);
                if (result < 0) break;
            } else {
                if (had_read_only) {
                    // Tell requester_id that it has PROT_READ_WRITE access
                    struct Message m = create_message(SET_PERMISSION,
                            MODIFYING, page_number);
                    m.is_response = 1;
                    if (request_id == NO_RESPONSE) {
                        result = E_INCONSISTENT_STATE;
                        break;
                    }
                    m.index = request_id;
                    result = send_to(requester_id, &m);
                    if (result < 0) break;
                } else {
                    // Transfer page over network to requester_id and tell it
                    //   it has PROT_READ_WRITE access
                    struct Message m = create_message(SEND_PAGE, MODIFYING,
                            page_number);
                    memcpy((void *) m.page, (void *) get_pageaddr(page_number),
                            PGSIZE);
                    m.is_response = 1;
                    if (request_id == NO_RESPONSE) {
                        result = E_INCONSISTENT_STATE;
                        break;
                    }
                    m.index = request_id;
                    result = send_to(requester_id, &m);
                    if (result < 0) break;
                }
            }
            // Set local page to PROT_NONE
            result = set_permissions((void *) get_pageaddr(page_number),
                    PGSIZE, PROT_NONE);
            if (result < 0) break;
            page_status->status_by_owner[thisid] = INVALIDATED;
            result = 0;
            break;
        }
        case MODIFIED: {
            int modifier = page_status->modifying_owner;
            if (modifier == requester_id) {
                // The modifier should have read-write permissions, in which
                //   case a page not fault should not be thrown and this
                //   should never be called.
                result = -E_INCONSISTENT_STATE;
                break;
            }
            if (modifier == thisid) {
                // Set local page to PROT_NONE at end of method
            } else {
                // Tell modifier that it is INVALIDATED
                // Get modified page from modifier
                struct Message m = create_message(REQUEST_PAGE, INVALIDATED,
                        page_number);
                // Upon receiving the message we should map the page locally
                result = send_and_wait_for_response(modifier, &m);
                if (result < 0) break;
                // Local page should already be set PROT_READ
            }
            page_status->status_by_owner[modifier] = INVALIDATED;
            page_status->status_by_owner[requester_id] = MODIFYING;
            page_status->modifying_owner = requester_id;
            if (requester_id == thisid) {
                // Set local page to PROT_READ_WRITE
                result = set_permissions((void *) get_pageaddr(page_number),
                        PGSIZE, PROT_READ_WRITE);
                if (result < 0) break;
            } else {
                // Transfer page over network to requester_id and tell it it has
                //   PROT_READ_WRITE access
                struct Message m = create_message(SEND_PAGE, MODIFYING,
                        page_number);
                memcpy((void *) m.page, (void *) get_pageaddr(page_number),
                        PGSIZE);
                m.is_response = 1;
                if (request_id == NO_RESPONSE) {
                    result = E_INCONSISTENT_STATE;
                    break;
                }
                m.index = request_id;
                result = send_to(requester_id, &m);
                if (result < 0) break;
                // Set local page to PROT_NONE
                result = set_permissions((void *) get_pageaddr(page_number),
                        PGSIZE, PROT_NONE);
                if (result < 0) break;
            }
            result = 0;
            break;
        }
        default: {
            printf("Unhandled page status in give_write_copy.\n");
            exit(-E_UNHANDLED_PAGE_STATUS);
        }
    }
    pthread_mutex_unlock(&locks[page_number]);
    return result;
}

int
get_read_copy (int page_number) {
    printf("%d: get_read_copy on page_number: %d\n", thisid, page_number);
    int result;
    pthread_mutex_lock(&locks[page_number]);
    int owner_id = get_owner(page_number);
    if (owner_id == thisid) {
        // Ask myself for read-ownership
        printf("%d: Asking myself for read-ownership of page %d\n", thisid,
                page_number);
        pthread_mutex_unlock(&locks[page_number]);
        return give_read_copy(thisid, NO_RESPONSE, page_number);
    } else {
        // Ask for read-ownership over network
        printf("%d: Asking %d for read-ownership of page %d\n", thisid, owner_id,
                page_number);
        struct Message m = create_message(REQUEST_PERMISSION, READING,
                page_number);
        result = send_and_wait_for_response(owner_id, &m);
    }
    return result;
}

int
get_write_copy (int page_number) {
    printf("%d: get_write_copy on page_number: %d\n", thisid, page_number);
    int result;
    int owner_id = get_owner(page_number);
    pthread_mutex_lock(&locks[page_number]);
    if (owner_id == thisid) {
        // Ask myself for write-ownership
        printf("%d: Asking myself for write-ownership of page %d\n", thisid,
                page_number);
        pthread_mutex_unlock(&locks[page_number]);
        return give_write_copy(thisid, NO_RESPONSE, page_number);
    } else {
        // Ask for write-ownership over network
        printf("%d: Asking %d for write-ownership of page %d\n", thisid, owner_id,
                page_number);
        struct Message m = create_message(REQUEST_PERMISSION, MODIFYING,
                page_number);
        result = send_and_wait_for_response(owner_id, &m);
    }
    pthread_mutex_unlock(&locks[page_number]);
    return result;
}
int
quit_dsm_page_ownership () {
    int i;
    for (i = 0; i < nowners; i++) {
        free(page_statuses[i].status_by_owner);
        page_statuses[i].status_by_owner = NULL;
    }
    free(page_statuses);
    page_statuses = NULL;
    for (i = 0; i < npages; i++) {
        pthread_mutex_destroy(&locks[i]);
    }
    for (i = 0; i < npages; i++) {
        pthread_cond_destroy(&waits[i]);
    }
    pthread_mutex_destroy(wait_indices_index_lock);
    return 0;
}

int
receive_message (int sender_id, struct Message *m) {
    printf("%d: received message from %d of type %d\n", thisid, sender_id,
            m->msg_type);
    int result;
    switch (m->msg_type) {
        case (REQUEST_PAGE): {
            pthread_mutex_lock(&locks[m->page_number]);
            void *pageaddr = (void *) get_pageaddr(m->page_number);
            result = set_permissions(pageaddr, PGSIZE, m->permissions);
            if (result < 0) break;
            struct Message response = create_message(SEND_PAGE, READING,
                    m->page_number);
            if (m->index == NO_RESPONSE) {
                result = E_INCONSISTENT_STATE;
                break;
            }
            response.index = m->index;
            response.is_response = 1;
            memcpy((void *) response.page, pageaddr, PGSIZE);
            result = send_to(sender_id, &response);
            pthread_mutex_lock(&locks[m->page_number]);
            break;
        }
        case (SEND_PAGE): {
            pthread_mutex_lock(&locks[m->page_number]);
            void *pageaddr = (void *) get_pageaddr(m->page_number);
            result = set_permissions(pageaddr, PGSIZE, PROT_READ_WRITE);
            if (result < 0) break;
            memcpy(pageaddr, (void *) m->page, PGSIZE);
            result = set_permissions(pageaddr, PGSIZE, m->permissions);
            if (m->is_response) {
                pthread_cond_signal(&waits[m->index]);
            }
            pthread_mutex_unlock(&locks[m->page_number]);
            break;
        }
        case (SET_PERMISSION): {
            pthread_mutex_lock(&locks[m->page_number]);
            void *pageaddr = (void *) get_pageaddr(m->page_number);
            result = set_permissions(pageaddr, PGSIZE, m->permissions);
            if (m->is_response) {
                pthread_cond_signal(&waits[m->index]);
            }
            pthread_mutex_unlock(&locks[m->page_number]);
            break;
        }
        case (REQUEST_PERMISSION): {
            if (m->permissions = READING) {
                give_read_copy(sender_id, m->index, m->page_number);
            } else if (m->permissions = MODIFYING) {
                give_write_copy(sender_id, m->index, m->page_number);
            } else {
                result = -E_UNHANDLED_PAGE_STATUS;
                break;
            }
            break;
        }
        default: {
            printf("Unhandled message type in give_read_copy.\n");
            exit(-E_UNHANDLED_MESSAGE_TYPE);
        }
    }
    return result;
}
