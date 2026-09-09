#ifndef __PROC_H__
#define __PROC_H__

#include "kthread.h"
#include "sched.h"

#include "slab_allocator.h"

#include "util/list.h"
#include "util/ktypes.h"
#include "util/spinlock.h"
#include "util/string.h"

#define PROC_MAX_COUNT 4096

/* Process states */
typedef enum
{
    PROC_PENDING,                // Has threads that have not yet been run
    PROC_RUNNING,                // Has running threads
    PROC_DEAD                    // Exited, but not yet wait'ed
} proc_state_t;

/* Process descriptor */
typedef struct proc
{
    pid_t p_pid;                 // Process ID
    char p_name[MAX_STRING_LEN]; // Process name

    list_t p_threads;            // Threads list
    spinlock_t p_threads_lock;   // lock for threads list
    list_t p_children;           // Children list
    spinlock_t p_children_lock;  // lock for children list
    struct proc *p_pproc;        // Parent process

    list_link_t p_list_link;     // Link of list of all processes
    list_link_t p_child_link;    // Link on parent's list of children

    long p_status;               // Exit status
    proc_state_t p_state;        // Process state
} proc_t;

/********************************
 * Process Subsystem Attributes *
 *******************************/

// global curproc variable to track the current process set in globals.h

// maintain the active processes
list_t proc_list;
spinlock_t proc_list_lock;
pid_t next_pid;

// idleproc is stored globally to the process subsystem
// rather than in the process list as it exists per core
proc_t idleproc;

// pointer to the init process
proc_t *proc_initproc;

// allocator for process descriptors
slab_allocator_t proc_allocator;

/*************
 * Functions *
 ************/

/*
 * Initializes the process subsystem
 */
void proc_init(void);

/*
 * Initialize the special idleproc at system startup
 */
void proc_idleproc_init();

/*
 * Shuts down thread-related subsystems at system shutdown
 */
void initproc_finish();

/*
 * Allocate and initialize a new process
 * @param name: the name to give the newly created process
 * @return the newly created process
 */
proc_t *proc_create(const char *name);

/*
 * Free all resources associated with a process
 * @param proc: the process to destroy
 */
void proc_destroy(proc_t *proc);

/*
 * Finish the execution of the current process
 */
void proc_cleanup();


/*
 * Handles exiting of a thread running on the current process
 * @param retval: the exit code for the thread
 */
void proc_thread_exiting(void *retval);

/*
 * Stops another process from running again by cancelling
 * all of its threads
 * @param proc: the process to kill
 * @param status: the status the process should exit with
 */
void proc_kill(proc_t *proc, long status);

/*
 * Kills every process except for the idele process and
 * direct children of the idle process
 */
void proc_kill_all(void);

#endif // __PROC_H__
