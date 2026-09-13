#include "proc.h"

proc_t *curproc;

/*
 * Initialize process subsystem
 */
void proc_init() {
    // Initialize subsystem's global attributes
    list_init(&proc_list);          // List of all processes
    spinlock_init(&proc_list_lock); // Spinlock for list of all processes
    next_pid = 1;                   // initproc has pid 1
    // TODO: Not sure here?
    slab_allocator_init(&proc_allocator, sizeof(proc_t));
}

/*
 *   Initialize idle process (special process with no threads)
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

    // Initialize the links (never used)
    list_link_init(&idleproc.p_list_link, &idleproc);
    list_link_init(&idleproc.p_child_link, &idleproc);

    // Active immediately
    idleproc.p_state = PROC_RUNNING;

    // No threads running yet
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
    // Remember parent in case "curproc" changes
    proc_t *parent = curproc;

    // Allocate space for the new process
    proc_t *proc = slab_obj_alloc(proc_allocator);
    if (proc == NULL) {
        // TODO:
        // Out of memory - throw an error
        return NULL;
    }

    // INIT THE PROCESS

    spinlock_lock(&proc_list_lock);
    proc->p_pid = next_pid;
    next_pid++;
    spinlock_unlock(&proc_list_lock);

    strncpy(proc->p_name, name, MAX_STRING_LEN);

    proc->p_status = 0; // Default status?
    proc->p_state = PROC_PENDING;

    list_init(&(proc->p_threads));
    list_init(&(proc->p_children));

    spinlock_init(&(proc->p_threads_lock));
    spinlock_init(&(proc->p_children_lock));

    proc->p_pproc = parent;

    list_link_init(&(proc->p_list_link), proc);
    list_link_init(&(proc->p_child_link), proc);

    // Add new proc to parent's list (synchronize)
    spinlock_lock(&(parent->p_children_lock));
    list_insert_back(&(parent->p_children), &(proc->p_child_link));
    spinlock_unlock(&(parent->p_children_lock));

    // Add new proc to global list of procs (synchronize)
    spinlock_lock(&proc_list_lock);
    list_insert_back(&proc_list, &(proc->p_list_link));
    spinlock_unlock(&proc_list_lock);

    return proc;
}

/*
 * TODO: implement me!
 * Hints: anything that was allocated needs to be deallocated... deallocated
 * objects should not be accessible by anyone else!
 *
 * Destroy a process by removing from all lists and freeing memory
 */
void proc_destroy(proc_t *proc) {
    // Remove from parents' children list
    proc_t *parent = proc->p_pproc;
    if (parent != NULL) {
        spinlock_lock(&(parent->p_children_lock));
        list_remove_link(&(parent->p_children), &proc->p_child_link);
        spinlock_unlock(&(parent->p_children_lock));
    }

    // Remove from list of all processes
    spinlock_lock(&proc_list_lock);
    list_remove_link(&proc_list, &(proc->p_list_link));
    spinlock_unlock(&proc_list_lock);

    // Free proc descriptor
    slab_obj_free(proc_allocator, proc);
}

/*
 * TODO: implement me!
 * Hints: anything that was allocated needs to be deallocated... deallocated
 * objects should not be accessible by anyone else!
 */
void proc_cleanup() {
    // Mark current process as dead
    curproc->p_state = PROC_DEAD;
    // TODO: is this it?
}

/*
 * TODO: implement me!
 * Hints: how should a process behave if all threads exit?
 */
void proc_thread_exiting(void *retval) {
    // Remember thread in case "curthr" changes
    kthread_t *thread = curthr;

    spinlock_lock(&(thread->kt_lock));
    thread->kt_retval = retval;
    thread->kt_state = KT_EXITED;
    spinlock_unlock(&(thread->kt_lock));
}

/*
 * TODO: implement me!
 * Hints:
 *   - cancel all threads associated with the provided process
 *   - protect access to the threads list
 */
void proc_kill(proc_t *proc, long status) {}

/*
 * TODO: implement me!
 * Hints:
 *  - protect access to the process list
 *  - kill the current process at the very end... don't kill before function
 * finishes!
 */
void proc_kill_all() {}
