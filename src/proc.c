#include "proc.h"

proc_t *curproc;

/*
 * Initialize process subsystem
 */
void proc_init() {
    // Initialize subsystem's global attributes
    list_init(&proc_list);          // List of all processes
    spinlock_init(&proc_list_lock); // Spinlock for list of all processes
    next_pid = 1;                   // pid 0 is reserved for the idleproc
    slab_allocator_init(&proc_allocator, sizeof(proc_t));
}

/*
 * Initialize idle process (special process with no threads)
 */
void proc_idleproc_init() {
    // Initialize pid and name
    idleproc.p_pid = 0;
    strncpy(idleproc.p_name, "idle", MAX_STRING_LEN);
    idleproc.p_status = 0;

    // Initialize lists
    list_init(&idleproc.p_threads); // Not used
    list_init(&idleproc.p_children);

    // Initialize locks
    spinlock_init(&idleproc.p_threads_lock); // Not used
    spinlock_init(&idleproc.p_children_lock);

    // No parent
    idleproc.p_pproc = NULL;

    // Initialize the links
    list_link_init(&idleproc.p_list_link, &idleproc); // Never used
    list_link_init(&idleproc.p_child_link, &idleproc);

    // Active immediately
    // TODO: or pending?
    idleproc.p_state = PROC_RUNNING;

    // The idleproc has no threads
    curproc = &idleproc;
    curthr = NULL;
}

/*
 * This function is implemented to tell the system to shut down and exit
 */
void initproc_finish() { context_switch(&curthr->kt_ctx, &bios_ctx); }

/*
 * Create a new process as a child of the currently running process
 */
proc_t *proc_create(const char *name) {
    // Remember the parent in case "curproc" changes
    proc_t *parent = curproc;

    // Allocate space for the new process
    proc_t *proc = slab_obj_alloc(proc_allocator);
    if (proc == NULL) {
        // Out of memory
        return NULL;
    }

    // INIT THE PROCESS
    strncpy(proc->p_name, name, MAX_STRING_LEN);

    proc->p_status = 0;
    proc->p_state = PROC_PENDING; // No threads have run yet

    list_init(&(proc->p_threads));
    list_init(&(proc->p_children));

    spinlock_init(&(proc->p_threads_lock));
    spinlock_init(&(proc->p_children_lock));

    proc->p_pproc = parent;

    list_link_init(&(proc->p_list_link), proc);
    list_link_init(&(proc->p_child_link), proc);

    // Assign pid and add to proc list (synchronize)
    spinlock_lock(&proc_list_lock);
    if (proc_list.size >= PROC_MAX_COUNT) {
        spinlock_unlock(&proc_list_lock);
        slab_obj_free(proc_allocator, proc);
        return NULL;
    }
    proc->p_pid = next_pid;
    next_pid++;
    list_insert_back(&proc_list, &(proc->p_list_link));
    spinlock_unlock(&proc_list_lock);

    // Add new proc to parent's list of children (synchronize)
    spinlock_lock(&(parent->p_children_lock));
    list_insert_back(&(parent->p_children), &(proc->p_child_link));
    spinlock_unlock(&(parent->p_children_lock));

    return proc;
}

/*
 * Destroy a process by freeing its threads, unlinking it from every list that
 * refers to it, and freeing the descriptor itself
 *
 * The process must already be dead and must not be the running process
 */
void proc_destroy(proc_t *proc) {
    if (proc == NULL || proc == curproc || proc == &idleproc) {
        return;
    }

    // Free every thread the process owns
    // kthread_destroy unlinks the thread from p_threads
    spinlock_lock(&(proc->p_threads_lock));
    while (proc->p_threads.size > 0) {
        kthread_t *thread = (kthread_t *)proc->p_threads.head->parent;
        spinlock_unlock(&(proc->p_threads_lock));

        // Unlock because kthread_destroy uses the same lock
        kthread_destroy(thread);

        spinlock_lock(&(proc->p_threads_lock));
    }
    spinlock_unlock(&(proc->p_threads_lock));

    // Remove from the parent's list of children (synchronize)
    proc_t *parent = proc->p_pproc;
    if (parent != NULL) {
        spinlock_lock(&(parent->p_children_lock));
        list_remove_link(&(parent->p_children), &(proc->p_child_link));
        spinlock_unlock(&(parent->p_children_lock));
    }

    // Remove from the list of all processes (synchronize)
    spinlock_lock(&proc_list_lock);
    list_remove_link(&proc_list, &(proc->p_list_link));
    spinlock_unlock(&proc_list_lock);

    // Free the process descriptor
    slab_obj_free(proc_allocator, proc);
}

/*
 * Helper function to hand off a dying process' children
 *
 */
static void proc_orphan_children(proc_t *proc) {
    // The initproc adopts everyone, except when the initproc is the one dying,
    // in which case its children fall back to the idleproc
    proc_t *reaper = proc_initproc;
    if (proc == proc_initproc) {
        reaper = &idleproc;
    }

    // Detach the whole list
    spinlock_lock(&(proc->p_children_lock));
    list_t orphans = proc->p_children;
    list_init(&(proc->p_children));
    spinlock_unlock(&(proc->p_children_lock));

    // Attach orphans to init/idle proc
    list_link_t *link;
    while ((link = list_remove_front(&orphans)) != NULL) {
        proc_t *child = (proc_t *)link->parent;
        child->p_pproc = reaper;

        spinlock_lock(&reaper->p_children_lock);
        list_insert_back(&reaper->p_children, &child->p_child_link);
        spinlock_unlock(&reaper->p_children_lock);
    }
}

/*
 * Finish the execution of the current process
 *
 * The descriptor is not freed here
 * It is the parent's job to call proc_destroy afterwards
 */
void proc_cleanup() {
    proc_t *proc = curproc;

    // No more threads will run for this process
    proc->p_state = PROC_DEAD;

    // Somebody still has to be around to clean up after our children
    proc_orphan_children(proc);

    // If initproc dies -- kill the system
    if (proc == proc_initproc) {
        initproc_finish();
    }
}

/*
 * Handles exiting of a thread running on the current process
 */
void proc_thread_exiting(void *retval) {
    // Remember the thread and process in case "curthr"/"curproc" change
    kthread_t *thread = curthr;
    proc_t *proc = curproc;

    // Set the return value and retire the thread (synchronize)
    spinlock_lock(&(thread->kt_lock));
    thread->kt_retval = retval;
    thread->kt_state = KT_EXITED;
    spinlock_unlock(&(thread->kt_lock));

    // If all threads exited, kill the process (synchronize)
    spinlock_lock(&(proc->p_threads_lock));

    long size = (proc->p_threads).size;
    list_link_t *link = proc->p_threads.head;

    while (size > 0) {
        kthread_t *sibling = (kthread_t *)link->parent;

        if (sibling->kt_state != KT_EXITED) {
            spinlock_unlock(&(proc->p_threads_lock));
            sched_switch();
            return; // Doesn't actually return ?!
        }

        link = link->next;
        size--;
    }
    spinlock_unlock(&(proc->p_threads_lock));

    // All threads exited
    proc_cleanup();

    // Schedule the next thread/process
    sched_switch();
}

/*
 * Stops another process from running again by cancelling all of its threads
 */
void proc_kill(proc_t *proc, long status) {
    // The idleproc can't be killed
    if (proc == NULL || proc == &idleproc || proc->p_state == PROC_DEAD) {
        return;
    }

    proc->p_status = status;

    // Cancel every thread except our own (synchronize)
    // curthr is left for last, because cancelling it exits
    spinlock_lock(&(proc->p_threads_lock));
    for (list_link_t *link = proc->p_threads.head; link != NULL;
         link = link->next) {
        kthread_t *thread = (kthread_t *)link->parent;
        if (thread != curthr) {
            kthread_cancel(thread, (void *)status);
        }
    }
    spinlock_unlock(&(proc->p_threads_lock));

    // Kill ourselves at last ?:)
    if (proc == curproc) {
        kthread_cancel(curthr, (void *)status);
        return;
    }

    // Mark process as dead
    proc->p_state = PROC_DEAD;

    // Hand off orphans (as they might still be alive)
    proc_orphan_children(proc);
}

/*
 * Kills every process except for the idle process and direct children of the
 * idle process
 */
void proc_kill_all() {
    // Kill all processes (synchronize)
    spinlock_lock(&proc_list_lock);
    list_link_t *link = proc_list.head;
    while (link != NULL) {
        list_link_t *next = link->next;
        proc_t *proc = (proc_t *)link->parent;
        if (proc != curproc && proc != &idleproc &&
            proc->p_pproc != &idleproc) {
            proc_kill(proc, 0);
        }
        link = next;
    }
    spinlock_unlock(&proc_list_lock);

    // Kill curproc at the end
    if (curproc != &idleproc && curproc->p_pproc != &idleproc) {
        proc_kill(curproc, 0);
    }
}
