#include "proc.h"

proc_t *curproc;

/*
 * TODO: implement me!
 * Hints: we don't have any processes running yet... but what from the process
 * subsystem needs to be initialized?
 */
void proc_init() {

}

/*
 * TODO: implement me!
 * The idle process is a special process that is created by kmain
 * its job is to be the first process on the system, but it does not have any
 * associated threads
 * Hints:
 *   - what would the fields of the process struct be set to for idleproc?
 *   - what is the initial value of curproc? curthr? 
 */
void proc_idleproc_init() {

}

/*
 * This function is implemented to tell the system to shut down and exit
 */
void initproc_finish() {
    context_switch(&curthr->kt_ctx, &bios_ctx);
}

/*
 * TODO: implement me!
 * Hints:
 *   - make space for the new process using the process allocator
 *   - we need to update the global structures
 *   - the process becomes a child of the current process
 *   - don't forget to synchronize on shared structures!
 */
proc_t *proc_create(const char *name) {
    return NULL;
}

/*
 * TODO: implement me!
 * Hints: anything that was allocated needs to be deallocated... deallocated
 * objects should not be accessible by anyone else!
 */
void proc_destroy(proc_t *proc) {

}

/*
 * TODO: implement me!
 * Hints: anything that was allocated needs to be deallocated... deallocated
 * objects should not be accessible by anyone else!
 */
void proc_cleanup() {

}

/*
 * TODO: implement me!
 * Hints: how should a process behave if all threads exit?
 */
void proc_thread_exiting(void *retval) {

}

/*
 * TODO: implement me!
 * Hints:
 *   - cancel all threads associated with the provided process
 *   - protect access to the threads list
 */
void proc_kill(proc_t *proc, long status) {

}

/*
 * TODO: implement me!
 * Hints:
 *  - protect access to the process list
 *  - kill the current process at the very end... don't kill before function
 * finishes!
 */
void proc_kill_all() {

}
