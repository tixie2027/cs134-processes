#ifndef __SCHED_H__
#define __SCHED_H__

#include "util/list.h"
#include "util/spinlock.h"

/**************
 * Structures *
 *************/

typedef struct ktqueue {
    list_t tq_list;
    spinlock_t tq_lock;
} ktqueue_t;

struct kthread; // forward declaration

ktqueue_t kt_runq; // queue of threads waiting to be run

/*************
 * Functions *
 ************/

/*
 * Initializes the scheduling subsystem
 */
void sched_init(void);

/*
 * Switch the current thread to a
 */
void sched_switch(void);

#endif // __SCHED_H__
