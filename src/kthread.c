#include "kthread.h"

kthread_t *curthr;

/*
 * TODO: implement me!
 * Hints: we don't have any threads running yet... but what from the thread
 * subsystem needs ot be initialized?
 */
void kthread_init() {

}


/*
 * TODO: implement me!
 * Hints:
 *   - make space for the new thread using the kthread allocator
 *   - set default values for thread fields
 *   - you will need to allocate a kernel stack
 *   - you will need to set up the thread's context
 *     --> for now, the page table for the process is NULL
 *   - remember to add the thread to the proc's p_thread list
 *   - initialize the kt_recent_core to ~0UL (unsigned -1)
 *   - return NULL if allocation not possible
 */
kthread_t *kthread_create(proc_t *proc, kthread_func_t func, long arg1,
                          void *arg2) {
    return NULL;
}

/*
 * TODO: implement me!
 * Hints:
 *   - the only parts of the context that must be initialized are c_kstack and
 *     c_kstacksz
 *   - the thread's process should be set outside of this function
 *   - copy over the retval, errno, and cancelled... other fields should be
 *     freshly initialized
 *   - remember to protect access to the thread via its spinlock
 *   - see kthread_create for more hints!
 */
kthread_t *kthread_clone(kthread_t *thread) {
    return NULL;
}

/*
 * TODO: implement me!
 * Hints:
 *   - deallocate thread memory
 *   - remove thread from process' thread list
 *   - protect all accesses to shared data
 *   - don't forget to free thread's stack!
 */
void kthread_destroy(kthread_t *thread) {

}

/*
 * TODO: implement me!
 * Hints:
 *   - cannot "cancel" the current thread, so call exit
 *   - mark the thread as cancelled and stop executing
 *   - remember to the protect access to the thread
 */
void kthread_cancel(kthread_t *thread, void *retval) {

}

/*
 * TODO: implement me!
 * Hints: there's (some but) not much to do here... remember, it's up to the
 * parent process to manage its threads!
 */
void kthread_exit(void *retval) {

}
