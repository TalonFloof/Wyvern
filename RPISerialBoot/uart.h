#ifndef _UART_H
#define _UART_H
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

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
int uart_getc_timeout(uint32_t timeout_ms);
void uart_puts(const char *s);
#endif