#include <stdint.h>
#include "uart.h"

/* The Raspberry Pi actually normally loads kernel8.bin in this position
 * We want the bootloader to be position independent so the kernel doesn't have to relocate itself
 */
#define LOAD_ADDR ((uint8_t *)0x100000)

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

static void receive_payload(void) {
    uint32_t size;
    uint8_t *dst = LOAD_ADDR;

    /* Signal ready to host — send three 0x03 bytes
     * Host waits for this before sending anything
     */
    uart_putc(READY_BYTE);
    uart_putc(READY_BYTE);
    uart_putc(READY_BYTE);

    /* Receive payload size (4 bytes, little-endian) */
    size = uart_get32();

    /* Sanity check — reject obviously bad sizes
     * Max we'll accept is 32MB which is more than enough
     */
    if (size == 0 || size > 0x2000000) {
        uart_puts("error: bad size\r\n");
        return;
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