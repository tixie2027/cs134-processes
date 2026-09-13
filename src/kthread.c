#include "kthread.h"

kthread_t *curthr;

/*
 * Initialize kernel thread subsystem
 */
void kthread_init() {
    // Thread allocator
    slab_allocator_init(&kthread_allocator, sizeof(kthread_t));
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
    // Allocate space for the new thread
    kthread_t *thread = slab_obj_alloc(kthread_allocator);
    if (thread == NULL) {
        // TODO:
        // Out of memory - throw an error
        return NULL;
    }

    // INIT THE THREAD
    thread->kt_state = KT_NO_STATE; // Not initialized yet

    thread->kt_proc = proc;   // Process that owns this thread
    thread->kt_cancelled = 0; // False

    // Allocate kernel stack
    thread->kt_kstack = page_alloc_n(DEFAULT_STACK_SIZE_PAGES);

    // TODO: Are these good defaults??
    thread->kt_retval = NULL;
    thread->kt_errno = 0;

    // Initialize the lock
    spinlock_init(&(thread->kt_lock));

    // Initialize the links
    list_link_init(&(thread->kt_plink), thread);
    list_link_init(&(thread->kt_qlink), thread);

    // Set up the context for this thread
    // TODO: Change page table from NULL to something else
    context_setup(&(thread->kt_ctx), func, arg1, arg2, thread->kt_kstack,
                  PAGE_SIZE * DEFAULT_STACK_SIZE_PAGES, NULL);

    // Add new thread to proc's kthread list (synchronize)
    spinlock_lock(&(proc->p_threads_lock));
    list_insert_back(&(proc->p_threads), &(thread->kt_plink));
    spinlock_unlock(&(proc->p_threads_lock));

    // TODO: Do I need to add it to the queue here?
    // Or is this done after I return a thread?
    // Add thread to the scheduling queue
    // spinlock_lock(&(kt_runq.tq_lock));
    // list_insert_back(&(kt_runq.tq_list), &(thread->kt_qlink));
    // spinlock_unlock(&(kt_runq.tq_lock));
    //
    // TODO:
    // Shouldn't run yet? can change to rannable
    thread->kt_state = KT_WAITING;

    return thread;
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
    kthread_t *new_thread = slab_obj_alloc(kthread_allocator);
    if (new_thread == NULL) {
        // TODO:
        // Out of memory - throw an error
        return NULL;
    }

    // INIT THE THREAD
    new_thread->kt_state = KT_NO_STATE; // Not initialized yet

    new_thread->kt_proc = NULL; // TODO: Is this a good default?
    new_thread->kt_cancelled = thread->kt_cancelled;

    new_thread->kt_retval = thread->kt_retval;
    new_thread->kt_errno = thread->kt_errno;

    // Allocate kernel stack
    new_thread->kt_kstack = page_alloc_n(DEFAULT_STACK_SIZE_PAGES);

    // Initialize the lock
    spinlock_init(&(new_thread->kt_lock));

    // Initialize the links
    list_link_init(&(new_thread->kt_plink), new_thread);
    list_link_init(&(new_thread->kt_qlink), new_thread);

    // Set up the context for this thread
    new_thread->kt_ctx.c_kstack = new_thread->kt_kstack;
    new_thread->kt_ctx.c_kstacksz = PAGE_SIZE * DEFAULT_STACK_SIZE_PAGES;

    // TODO: How do we copy a context, do we need to copy the context?
    // TODO: Change page table from NULL to something else
    // context_setup(&(new_thread->kt_ctx), func, arg1, arg2, thread->kt_kstack,
    //               PAGE_SIZE * DEFAULT_STACK_SIZE_PAGES, NULL);

    // TODO: Does this need to happen outside the function?
    // new_thread->kt_proc = thread->kt_proc; // Same as cloner?
    // Add new thread to proc's kthread list (synchronize)
    // proc_t *proc = new_thread->kt_proc;
    // spinlock_lock(&(proc->p_threads_lock));
    // list_insert_back(&(proc->p_threads), &(new_thread->kt_plink));
    // spinlock_unlock(&(proc->p_threads_lock));

    // TODO: Do I need to add it to the queue here?
    // Or is this done after I return the thread?
    // Add thread to the scheduling queue
    // spinlock_lock(&(kt_runq.tq_lock));
    // list_insert_back(&(kt_runq.tq_list), &(thread->kt_qlink));
    // spinlock_unlock(&(kt_runq.tq_lock));
    //
    // TODO:
    // Shouldn't run yet? can change to runnable
    new_thread->kt_state = KT_WAITING;

    return new_thread;
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
    // Remove thread from process' thread list
    proc_t *proc = thread->kt_proc;

    spinlock_lock(&(proc->p_threads_lock));
    list_remove_link(&(proc->p_threads), &(thread->kt_plink));
    spinlock_unlock(&(proc->p_threads_lock));

    // Free thread's stack
    page_free_n(thread->kt_kstack, DEFAULT_STACK_SIZE_PAGES);

    // Free thread descriptor
    slab_obj_free(kthread_allocator, thread);

    // TODO: Do I need to remove it from the sched queue?
}

/*
 * TODO: implement me!
 * Hints:
 *   - cannot "cancel" the current thread, so call exit
 *   - mark the thread as cancelled and stop executing
 *   - remember to the protect access to the thread
 */
void kthread_cancel(kthread_t *thread, void *retval) {
    if (thread == curthr) {
        kthread_exit(retval);
        return;
    }

    spinlock_lock(&thread->kt_lock);

    thread->kt_cancelled = 1; // True
    thread->kt_retval = retval;
    thread->kt_state = KT_EXITED; // TODO: Not sure about this

    spinlock_unlock(&thread->kt_lock);
}

/*
 * TODO: implement me!
 * Hints: there's (some but) not much to do here... remember, it's up to the
 * parent process to manage its threads!
 *
 * Exit currently running thread
 */
void kthread_exit(void *retval) {
    // Let the process subsystem handle this
    proc_thread_exiting(retval);
}
