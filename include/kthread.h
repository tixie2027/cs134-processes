#ifndef __KTHREAD_H__
#define __KTHREAD_H__

#include "context.h"
#include "sched.h"

#include "slab_allocator.h"

#include "util/list.h"
#include "util/ktypes.h"
#include "util/spinlock.h"

#include "globals.h"

#define DEFAULT_STACK_SIZE_PAGES 16

/* Thread states */
typedef enum {
    KT_NO_STATE,          // illegal state
    KT_ON_CPU,            // currently running
    KT_RUNNABLE,          // on the run queue and runnable
    KT_WAITING,           // on the run queue and should not be run
    KT_EXITED             // exited, waiting to be joined
} kthread_state_t;

/* Thread descriptor */
typedef struct kthread {
    char *kt_kstack;          // kernel stack pointer
    void *kt_retval;          // return value
    long kt_errno;            // errno of most recent syscall
    context_t kt_ctx;         // thread context

    struct proc *kt_proc;     // corresponding process

    long kt_cancelled;        // set if the thread has been cancelled
    kthread_state_t kt_state; // thread state

    spinlock_t kt_lock;       // so thread state doesn't change during clone

    list_link_t kt_plink;     // link on process's thread list, p_threads
    list_link_t kt_qlink;     // link on ktqueue if thread isn't running
} kthread_t;

/*******************************
 * Thread Subsystem Attributes *
 ******************************/

// global curthr variable to track the current thread declared in globals

// allocator for process descriptors
slab_allocator_t kthread_allocator;


/*************
 * Functions *
 ************/

/*
 * Initializes the kthread subsystem at system startup.
 */
void kthread_init(void);

/*
 * Allocates and initializes a kernel thread
 *
 * @param proc: the process in which the thread will run
 * @param func: the function that will be called by the newly created thread
 * @param arg1: the first argument to func
 * @param arg2: the second argument to func
 * @return: the newly created thread
 */
kthread_t *kthread_create(struct proc *proc, kthread_func_t func, long arg1,
                          void *arg2);

/*
 * Creates a clone of the specified thread
 *
 * @param thread: the thread to clone
 * @return: the newly created thread
 */
kthread_t *kthread_clone(kthread_t *thread);

/*
 * Frees resources associated with a thread
 *
 * @param thread: the thread to free
 */
void kthread_destroy(kthread_t *thread);

/*
 * Cancels a thread
 *
 * @param thread: the thread to be canceled
 * @param retval: the reutrn value for the thread
 */
void kthread_cancel(kthread_t *thread, void *retval);

/*
 * Exits the current thread
 *
 * @param retval: the return value for the thread
 */
void kthread_exit(void *retval);

#endif // __KTHREAD_H__
