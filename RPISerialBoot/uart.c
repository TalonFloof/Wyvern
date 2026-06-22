#include <stdint.h>
#include "uart.h"

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
    //*UART0_IBRD = 26;
    //*UART0_FBRD = 3;
    *UART0_IBRD = 2;
    *UART0_FBRD = 0;

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

int uart_getc_timeout(uint32_t timeout_ms) {
    volatile uint32_t count = timeout_ms * 10000;
    while (count > 0) {
        if (!(*UART0_FR & FR_RXFE))
            return (int)(*UART0_DR & 0xFF);
        count--;
    }
    return -1;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}