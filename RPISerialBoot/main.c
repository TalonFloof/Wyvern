#include <stdint.h>
#include "uart.h"

/* The Raspberry Pi actually normally loads kernel8.bin in this position
 * We want the bootloader to be position independent so the kernel doesn't have to relocate itself
 */
#define LOAD_ADDR ((uint8_t *)0x80000)

#define READY_BYTE  0x03
#define ACK_O       0x4F /* 'O' */
#define ACK_K       0x4B /* 'K' */

static uint32_t uart_get32(void) {
    uint32_t val = 0;
    /* Receive 4 bytes little-endian */
    val  = (uint32_t)uart_getc();
    val |= (uint32_t)uart_getc() << 8;
    val |= (uint32_t)uart_getc() << 16;
    val |= (uint32_t)uart_getc() << 24;
    return val;
}

static void uart_flush_rx(void) {
    // Drain any bytes sitting in the RX FIFO
    while (!(*UART0_FR & FR_RXFE)) {
        (void)*UART0_DR;    // read and discard
    }
}

static void receive_payload(void) {
    uint32_t size;
    uint8_t *dst = LOAD_ADDR;

    while (1) {
        uart_flush_rx();

        // Send ready signal
        uart_putc(READY_BYTE);
        uart_putc(READY_BYTE);
        uart_putc(READY_BYTE);

        // Wait for a response with timeout
        // At 115200 baud, one byte takes ~87us
        // We'll wait ~3 seconds worth of iterations
        volatile uint32_t timeout = 3000000;
        while (timeout > 0 && !(*UART0_FR & FR_RXFE)) {
            timeout--;
        }

        // Nothing came in, retry
        if (timeout == 0) {
            continue;
        }

        // Got something — read the 4 byte size
        size = uart_get32();

        // Validate size
        if (size == 0 || size > 0x2000000) {
            uart_puts("error: bad size\r\n");
            continue;    // retry instead of return
        }

        // Valid size received, break out
        break;
    }

    /* Acknowledge size with "OK" */
    uart_putc(ACK_O);
    uart_putc(ACK_K);

    /* Receive payload bytes */
    for (uint32_t i = 0; i < size; i++) {
        dst[i] = (uint8_t)uart_getc();
    }

    uart_puts("info: payload transfer finished, jumping to kernel\r\n");

    /* Clean up UART — drain TX FIFO before jumping */
    for (volatile int i = 0; i < 10000; i++);

    /* Cast load address to a function pointer and jump
     * This transfers control to the received payload
     */
    void (*payload)(void) = (void (*)(void))LOAD_ADDR;
    payload();
}

void main(void) {
    uart_init();
    uart_puts("info: Wyvern Serial Bootloader for rpi3\r\n");

    /* Loop forever — if payload returns or receive fails,
     * wait for next upload rather than hanging
     */
    while (1) {
        receive_payload();
    }
}