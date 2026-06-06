// payload/main.c
#include <stdint.h>

#define MMIO_BASE       0x3F000000

#define GPFSEL1         ((volatile uint32_t *)(MMIO_BASE + 0x200004))
#define GPPUD           ((volatile uint32_t *)(MMIO_BASE + 0x200094))
#define GPPUDCLK0       ((volatile uint32_t *)(MMIO_BASE + 0x200098))

#define AUX_ENABLES     ((volatile uint32_t *)(MMIO_BASE + 0x215004))
#define AUX_MU_IO       ((volatile uint32_t *)(MMIO_BASE + 0x215040))
#define AUX_MU_IER      ((volatile uint32_t *)(MMIO_BASE + 0x215044))
#define AUX_MU_IIR      ((volatile uint32_t *)(MMIO_BASE + 0x215048))
#define AUX_MU_LCR      ((volatile uint32_t *)(MMIO_BASE + 0x21504C))
#define AUX_MU_MCR      ((volatile uint32_t *)(MMIO_BASE + 0x215050))
#define AUX_MU_LSR      ((volatile uint32_t *)(MMIO_BASE + 0x215054))
#define AUX_MU_CNTL     ((volatile uint32_t *)(MMIO_BASE + 0x215060))
#define AUX_MU_BAUD     ((volatile uint32_t *)(MMIO_BASE + 0x215068))

#define AUX_MU_LSR_TX_EMPTY  (1 << 5)
#define AUX_MU_LSR_RX_READY  (1 << 0)

static void delay(int32_t count) {
    asm volatile("__delay_%=: subs %[count], %[count], #1\n"
                 "bne __delay_%=\n"
                 : [count] "+r" (count));
}

static void uart_init(void) {
    *AUX_ENABLES = 1;
    *AUX_MU_CNTL = 0;
    *AUX_MU_IER  = 0;
    *AUX_MU_LCR  = 3;
    *AUX_MU_MCR  = 0;
    *AUX_MU_IIR  = 0xC6;
    *AUX_MU_BAUD = 270;

    uint32_t selector = *GPFSEL1;
    selector &= ~(7 << 12);
    selector |=  (2 << 12);
    selector &= ~(7 << 15);
    selector |=  (2 << 15);
    *GPFSEL1 = selector;

    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUD     = 0;
    *GPPUDCLK0 = 0;

    *AUX_MU_CNTL = 3;
}

static void uart_putc(char c) {
    while (!(*AUX_MU_LSR & AUX_MU_LSR_TX_EMPTY));
    *AUX_MU_IO = c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void main(void) {
    uart_init();
    uart_puts("Hello from AArch64 payload!\n");
    uart_puts("Wyvern bootloader works end to end.\n");

    // Loop forever
    while (1) {
        asm volatile("wfe");
    }
}