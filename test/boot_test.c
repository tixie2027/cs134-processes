#include "proc.h"
#include <assert.h>

static context_t bootstrap_ctx;
static int init_ran;

static void *initproc_run(long arg1, void *arg2) {
    init_ran = 1;
    return NULL;
}

void *start_initproc(long arg1, void *arg2) {
    proc_initproc = proc_create("init");
    kthread_t *init_thread = kthread_create(proc_initproc, initproc_run, 0, NULL);

    curproc = proc_initproc;
    curthr = init_thread;

    context_make_active(&init_thread->kt_ctx);

    return NULL;
}

int main(int argc, char **argv) {
    if (mem_init() != 0) return -1;
    slab_init();
    proc_init();
    kthread_init();
    sched_init();
    proc_idleproc_init();

    void *bootstrap_stack = page_alloc_n(1);
    if (bootstrap_stack == NULL) {
        return -1;
    }

    context_setup(&bootstrap_ctx, start_initproc, 0, NULL, bootstrap_stack, PAGE_SIZE, NULL);
    context_switch(&bios_ctx, &bootstrap_ctx);

    assert(init_ran);
    assert(proc_initproc->p_state == PROC_DEAD);
    assert(proc_initproc->p_status == 0);
    assert(curthr->kt_state == KT_EXITED);
    assert(curthr->kt_retval == NULL);
    assert(!proc_list_lock.s_locked);
    assert(!proc_initproc->p_threads_lock.s_locked);

    return 0;
}
