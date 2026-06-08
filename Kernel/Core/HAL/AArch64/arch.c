#include "../hal.h"
#include <stdint.h>

static inline uint64_t read_currentel(void) {
    uint64_t val;
    asm volatile("mrs %0, CurrentEL" : "=r"(val));
    return val >> 2;
}

static inline void write_sctlr_el1(uint64_t val) {
    asm volatile("msr sctlr_el1, %0\n isb" :: "r"(val));
}

static inline uint64_t read_sctlr_el1(void) {
    uint64_t val;
    asm volatile("mrs %0, sctlr_el1" : "=r"(val));
    return val;
}

#define SCTLR_M         (1UL << 0)   /* MMU enable */
#define SCTLR_A         (1UL << 1)   /* Alignment fault enable */
#define SCTLR_C         (1UL << 2)   /* Data cache enable */
#define SCTLR_SA        (1UL << 3)   /* Stack alignment check EL1 */
#define SCTLR_SA0       (1UL << 4)   /* Stack alignment check EL0 */
#define SCTLR_I         (1UL << 12)  /* Instruction cache enable */
#define SCTLR_WXN       (1UL << 19)  /* Write implies execute never */
#define SCTLR_SPAN      (1UL << 23)  /* Set privileged access never */
#define SCTLR_EIS       (1UL << 22)  /* Exception entry is context synchronizing */
#define SCTLR_EOS       (1UL << 11)  /* Exception exit is context synchronizing */

#define HCR_RW          (1UL << 31)  /* EL1 is AArch64 */

#define SCR_NS          (1UL << 0)   /* Non-secure */
#define SCR_RW          (1UL << 10)  /* EL2 is AArch64 */

#define SPSR_EL1H       0x5          /* EL1h — EL1 with SP_EL1 */
#define SPSR_EL2H       0x9          /* EL2h — EL2 with SP_EL2 */
#define SPSR_DAIF_MASK  (0xF << 6)   /* Mask D, A, I, F */

static void hal_drop_to_el1() {
    uint64_t el = read_currentel();

    if (el == 1) {
        /* Already in EL1 */
        return;
    }

    if (el == 3) {
        /* Drop from EL3 to EL2 before we drop to EL1 */
        uint64_t scr = SCR_NS | SCR_RW;
        asm volatile("msr scr_el3, %0" :: "r"(scr));

        uint64_t spsr = SPSR_DAIF_MASK | SPSR_EL2H;
        asm volatile("msr spsr_el3, %0" :: "r"(spsr));

        asm volatile(
            "adr x0, 1f\n"
            "msr elr_el3, x0\n"
            "eret\n"
            "1:\n"
            ::: "x0"
        );
    }

    uint64_t hcr = HCR_RW;
    asm volatile("msr hcr_el2, %0" :: "r"(hcr));

    uint64_t sctlr = 0;
    sctlr |= SCTLR_EIS;
    sctlr |= SCTLR_EOS;
    asm volatile("msr sctlr_el1, %0" :: "r"(sctlr));

    uint64_t spsr = SPSR_DAIF_MASK | SPSR_EL1H;
    asm volatile("msr spsr_el2, %0" :: "r"(spsr));

    asm volatile(
        "adr x0, 1f\n"
        "msr elr_el2, x0\n"
        "eret\n"
        "1:\n"
        ::: "x0"
    );
}

void hal_early_init(void *dtb) {
    (void)dtb;  /* TODO: Use Device Tree */

    hal_drop_to_el1();

    uint64_t sctlr = read_sctlr_el1();
    sctlr &= ~SCTLR_M;     /* MMU off */
    sctlr &= ~SCTLR_C;     /* D-cache off */
    sctlr &= ~SCTLR_I;     /* I-cache off */
    sctlr &= ~SCTLR_A;     /* No alignment faults yet */
    sctlr &= ~SCTLR_WXN;   /* Don't enforce W^X yet */
    write_sctlr_el1(sctlr);

    /* Disable FP/SIMD trapping */
    asm volatile(
        "mrs x0, cpacr_el1\n"
        "orr x0, x0, #(3 << 20)\n"
        "msr cpacr_el1, x0\n"
        "isb\n"
        ::: "x0"
    );

    /* Disable Hardware Debugging */
    asm volatile(
        "msr mdscr_el1, xzr\n"
        "isb\n"
    );


}

int hal_irq_enable_disable(int enable) {
    int old;
    asm volatile("mrs %0, daif" : "=r"(old));
    if(!enable && !old) {
        asm volatile("msr daifset, #0xF\n isb");
    } else if(enable && old) {
        asm volatile("msr daifclr, #0xF\n isb");
    }
    return old;
}

void hal_fence() { asm volatile("dmb sy"  ::: "memory"); }

void hal_write_fence() { asm volatile("dmb st" ::: "memory"); }

void hal_read_fence() { asm volatile("dmb ld" ::: "memory"); }

void hal_sync_fence() { asm volatile("dsb sy"  ::: "memory"); }

void hal_inst_fence() { asm volatile("isb" ::: "memory"); }
