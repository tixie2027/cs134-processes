#include "context.h"
#include "kthread.h"
#include "sched.h"

void sched_init() {
    // initialize run queue
    list_init(&kt_runq.tq_list);
    spinlock_init(&kt_runq.tq_lock);
}

void sched_switch() {
    // get the next thread
    spinlock_lock(&kt_runq.tq_lock);
    if (kt_runq.tq_list.size == 0) {
        return; // uh oh! nobody else to run... just exit
    }

    context_t *old_ctx;
    kthread_t *next_thread;
    while (1) {
        old_ctx = &curthr->kt_ctx;
        next_thread = (kthread_t *) list_remove_front(&kt_runq.tq_list)->parent;

        // if next_thread is cancelled, don't run it...
        if (next_thread->kt_state != KT_RUNNABLE) {
            list_insert_back(&kt_runq.tq_list, &next_thread->kt_qlink);
        } else {
            break;
        }
    }

    // thread is descheduled
    if (curthr->kt_state == KT_ON_CPU) {
        curthr->kt_state = KT_RUNNABLE;

        // put thread back into runq
        list_insert_back(&kt_runq.tq_list, &curthr->kt_qlink);
    }

    spinlock_unlock(&kt_runq.tq_lock);


    // set curthr and curproc
    curthr = next_thread;
    curproc = curthr->kt_proc;

    curthr->kt_state = KT_ON_CPU;

    // no need to save the current thread, we assume it's run its course!
    // context switch to the next thread
    context_switch(old_ctx, &curthr->kt_ctx);
}
