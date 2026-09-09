#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "util/ktypes.h"

typedef void *page_table_t; // for now... we will come back to this!

typedef struct context {
    // general registers
    void *c_rax;
    void *c_rbx;
    void *c_rcx;
    void *c_rdx;

    // index and pointers
    void *c_rip;
    void *c_rsp;
    void *c_rbp;
    void *c_rsi;
    void *c_rdi;

    void *c_flags;

    page_table_t *page_table;

    char *c_kstack;
    unsigned long c_kstacksz;
} context_t;

context_t bios_ctx;

void context_setup(context_t *c, context_func_t func, long arg1, void *arg2,
                   void *kstack, int kstacksz, page_table_t *pt);

void context_make_active(context_t *c);

void context_switch(context_t *oldc, context_t *newc);

#endif // __CONTEXT_H__
