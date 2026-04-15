#include <stdint.h>
#include "../../main.h"
#include "../arch.h"

typedef struct WyvernRegFrame {
    uint64_t ip;      // Offset 0:  sepc (Instruction Pointer)
    uint64_t ra;      // Offset 8:  x1
    uint64_t sp;      // Offset 16: x2
    uint64_t gp;      // Offset 24: x3
    uint64_t tp;      // Offset 32: x4
    uint64_t t0;      // Offset 40: x5
    uint64_t t1;      // Offset 48: x6
    uint64_t t2;      // Offset 56: x7
    uint64_t s0;      // Offset 64: x8 (fp)
    uint64_t s1;      // Offset 72: x9
    uint64_t a0;      // Offset 80: x10 (Arg 0 / Return)
    uint64_t a1;      // Offset 88: x11 (Arg 1)
    uint64_t a2;      // Offset 96: x12 (Arg 2)
    uint64_t a3;      // Offset 104: x13
    uint64_t a4;      // Offset 112: x14
    uint64_t a5;      // Offset 120: x15
    uint64_t a6;      // Offset 128: x16
    uint64_t a7;      // Offset 136: x17 (Syscall ID)
    uint64_t s2;      // Offset 144: x18
    uint64_t s3;      // Offset 152: x19
    uint64_t s4;      // Offset 160: x20
    uint64_t s5;      // Offset 168: x21
    uint64_t s6;      // Offset 176: x22
    uint64_t s7;      // Offset 184: x23
    uint64_t s8;      // Offset 192: x24
    uint64_t s9;      // Offset 200: x25
    uint64_t s10;     // Offset 208: x26
    uint64_t s11;     // Offset 216: x27
    uint64_t t3;      // Offset 224: x28
    uint64_t t4;      // Offset 232: x29
    uint64_t t5;      // Offset 240: x30
    uint64_t t6;      // Offset 248: x31
} WyvernRegFrame;

void arch_trap_handler(WyvernRegFrame* frame) {
    uintptr_t scause;
    asm volatile("csrr %0, scause" : "=r"(scause));
    
    panic("uncaught exception trap, cause 0x%x @ 0x%p\n  ra %p  sp %p  gp %p\n  tp %p  t0 %p  t1 %p\n  t2 %p  t3 %p  t4 %p\n  t5 %p  t6 %p  s0 %p\n  s1 %p  s2 %p  s3 %p\n  s4 %p  s5 %p  s6 %p\n  s7 %p  s8 %p  s9 %p\n s10 %p s11 %p  a0 %p\n  a1 %p  a2 %p  a3 %p\n  a4 %p  a5 %p  a6 %p\n  a7 %p", scause, frame->ip,
        frame->ra, frame->sp, frame->gp, 
        frame->tp,frame->t0,frame->t1,
        frame->t2,frame->t3,frame->t4,
        frame->t5,frame->t6,frame->s0,
        frame->s1,frame->s2,frame->s3,
        frame->s4,frame->s5,frame->s6,
        frame->s7,frame->s8,frame->s9,
        frame->s10,frame->s11,frame->a0,
        frame->a1,frame->a2,frame->a3,
        frame->a4,frame->a5,frame->a6,
        frame->a7);
}
