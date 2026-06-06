#include <stdint.h>

#define MMIO_BASE       0x3F000000

// GPIO registers
#define GPFSEL1         ((volatile uint32_t *)(MMIO_BASE + 0x200004))
#define GPPUD           ((volatile uint32_t *)(MMIO_BASE + 0x200094))
#define GPPUDCLK0       ((volatile uint32_t *)(MMIO_BASE + 0x200098))

// PL011 UART registers
#define UART0_DR        ((volatile uint32_t *)(MMIO_BASE + 0x201000))
#define UART0_FR        ((volatile uint32_t *)(MMIO_BASE + 0x201018))
#define UART0_IBRD      ((volatile uint32_t *)(MMIO_BASE + 0x201024))
#define UART0_FBRD      ((volatile uint32_t *)(MMIO_BASE + 0x201028))
#define UART0_LCRH      ((volatile uint32_t *)(MMIO_BASE + 0x20102C))
#define UART0_CR        ((volatile uint32_t *)(MMIO_BASE + 0x201030))
#define UART0_IMSC      ((volatile uint32_t *)(MMIO_BASE + 0x201038))
#define UART0_ICR       ((volatile uint32_t *)(MMIO_BASE + 0x201044))

// Flag register bits
#define FR_TXFF         (1 << 5)    // TX FIFO full
#define FR_RXFE         (1 << 4)    // RX FIFO empty

static void delay(int32_t count) {
    asm volatile("__delay_%=: subs %[count], %[count], #1\n"
                 "bne __delay_%=\n"
                 : [count] "+r" (count));
}

void uart_init(void) {
    // Disable UART before reconfiguring
    *UART0_CR = 0;

    // Configure GPIO14 and GPIO15 for Alt0 (PL011)
    // Alt0 = 0b100
    uint32_t selector = *GPFSEL1;
    selector &= ~(7 << 12);     // clear GPIO14
    selector |=  (4 << 12);     // set Alt0 for GPIO14
    selector &= ~(7 << 15);     // clear GPIO15
    selector |=  (4 << 15);     // set Alt0 for GPIO15
    *GPFSEL1 = selector;

    // Disable pull up/down for GPIO14 and GPIO15
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUD = 0;
    *GPPUDCLK0 = 0;

    // Clear all pending interrupts
    *UART0_ICR = 0x7FF;

    // Set baud rate to 115200
    // PL011 baud rate divisor = UART clock / (16 * baud rate)
    // UART clock is 48MHz by default on BCM2837
    // 48000000 / (16 * 115200) = 26.041...
    // Integer part = 26
    // Fractional part = 0.041 * 64 = 2.604 → round to 3
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;

    // 8 bit, no parity, 1 stop bit, enable FIFO
    *UART0_LCRH = (1 << 4) | (1 << 5) | (1 << 6);

    // Mask all interrupts
    *UART0_IMSC = (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
                  (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);

    // Enable UART, TX and RX
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(char c) {
    // Wait until TX FIFO has space
    while (*UART0_FR & FR_TXFF);
    *UART0_DR = c;
}

char uart_getc(void) {
    // Wait until RX FIFO has data
    while (*UART0_FR & FR_RXFE);
    return (char)(*UART0_DR & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}