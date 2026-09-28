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
 * Allocate and initialize a kernel thread that will run func(arg1, arg2) in
 * the given process
 *
 */
kthread_t *kthread_create(proc_t *proc, kthread_func_t func, long arg1,
                          void *arg2) {
    if (proc == NULL) {
        return NULL;
    }

    // Allocate space for the new thread
    kthread_t *thread = slab_obj_alloc(kthread_allocator);
    if (thread == NULL) {
        // Out of memory
        return NULL;
    }

    // Allocate the kernel stack this thread will execute on
    thread->kt_kstack = page_alloc_n(DEFAULT_STACK_SIZE_PAGES);
    if (thread->kt_kstack == NULL) {
        // Out of memory
        slab_obj_free(kthread_allocator, thread);
        return NULL;
    }

    // INIT THE THREAD
    thread->kt_proc = proc; // Process that owns this thread
    thread->kt_cancelled = 0;

    // Nothing has run yet, so there is no return value and no syscall error
    thread->kt_retval = NULL;
    thread->kt_errno = 0;

    // Initialize the lock
    spinlock_init(&(thread->kt_lock));

    // Initialize the links
    list_link_init(&(thread->kt_plink), thread);
    list_link_init(&(thread->kt_qlink), thread);

    // Set up the context for this thread
    context_setup(&(thread->kt_ctx), func, arg1, arg2, thread->kt_kstack,
                  PAGE_SIZE * DEFAULT_STACK_SIZE_PAGES, NULL);

    // Ready to be scheduled, but not queued
    thread->kt_state = KT_RUNNABLE;

    // Add the new thread to the proc's kthread list (synchronize)
    spinlock_lock(&(proc->p_threads_lock));
    list_insert_back(&(proc->p_threads), &(thread->kt_plink));
    spinlock_unlock(&(proc->p_threads_lock));

    return thread;
}

/*
 * Create a clone of the specified thread
 *
 * Only c_kstack and c_kstacksz of the context are filled in
 * Caller is responsible for the rest of the context and for attaching the
 * clone to a process (kt_proc is left NULL here)
 */
kthread_t *kthread_clone(kthread_t *thread) {
    if (thread == NULL) {
        return NULL;
    }

    kthread_t *new_thread = slab_obj_alloc(kthread_allocator);
    if (new_thread == NULL) {
        // Out of memory
        return NULL;
    }

    // Allocate a kernel stack of its own
    new_thread->kt_kstack = page_alloc_n(DEFAULT_STACK_SIZE_PAGES);
    if (new_thread->kt_kstack == NULL) {
        // Out of memory
        slab_obj_free(kthread_allocator, new_thread);
        return NULL;
    }

    // INIT THE THREAD

    // Copy the state
    // (synchronize: the source thread must not change while we read it)
    spinlock_lock(&(thread->kt_lock));
    new_thread->kt_retval = thread->kt_retval;
    new_thread->kt_errno = thread->kt_errno;
    new_thread->kt_cancelled = thread->kt_cancelled;
    spinlock_unlock(&(thread->kt_lock));

    // Everything else is freshly initialized
    new_thread->kt_proc = NULL;         // Set by the caller
    new_thread->kt_state = KT_NO_STATE; // Set to KT_RUNNABLE by the caller

    // Initialize the lock
    spinlock_init(&(new_thread->kt_lock));

    // Initialize the links
    list_link_init(&(new_thread->kt_plink), new_thread);
    list_link_init(&(new_thread->kt_qlink), new_thread);

    // Point the context at the new stack
    new_thread->kt_ctx.c_kstack = new_thread->kt_kstack;
    new_thread->kt_ctx.c_kstacksz = PAGE_SIZE * DEFAULT_STACK_SIZE_PAGES;

    return new_thread;
}

/*
 * Free resources associated with a thread
 *
 * The thread must not be running and must not be on any queue
 */
void kthread_destroy(kthread_t *thread) {
    if (thread == NULL || thread == curthr) {
        return;
    }

    // Remove the thread from its process' thread list (synchronize)
    proc_t *proc = thread->kt_proc;
    if (proc != NULL) {
        spinlock_lock(&(proc->p_threads_lock));
        list_remove_link(&(proc->p_threads), &(thread->kt_plink));
        spinlock_unlock(&(proc->p_threads_lock));
    }

    // Free thread's stack
    page_free_n(thread->kt_kstack, DEFAULT_STACK_SIZE_PAGES);
    thread->kt_kstack = NULL;

    // Free the thread descriptor
    slab_obj_free(kthread_allocator, thread);
}

/*
 * Cancel a thread so that it never runs again
 */
void kthread_cancel(kthread_t *thread, void *retval) {
    if (thread == NULL) {
        return;
    }

    // Can't cancel a running thread
    if (thread == curthr) {
        kthread_exit(retval);
        return;
    }

    // Cancel the thread (synchronize)
    spinlock_lock(&(thread->kt_lock));
    thread->kt_cancelled = 1;
    thread->kt_retval = retval;
    thread->kt_state = KT_EXITED;
    spinlock_unlock(&(thread->kt_lock));
}

/*
 * Exit the currently running thread
 *
 */
void kthread_exit(void *retval) {
    // It is up to the process to manage its threads, so hand off to it
    proc_thread_exiting(retval);
}
