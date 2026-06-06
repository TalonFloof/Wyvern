#include <stdint.h>
#include "uart.h"
#include "crc32.h"

#define LOAD_ADDR ((uint8_t *)0x80000)

#define READY_BYTE      0x03
#define ACK_O           0x4F
#define ACK_K           0x4B
#define ACK             0x06
#define NAK             0x15
#define GO_G            0x47
#define GO_O            0x4F

#define CHUNK_SIZE      512
#define MAX_RETRIES     3

#define HART_MAILBOXES  ((volatile uint64_t *)0x1000)

static void wake_harts(void *entry) {
    HART_MAILBOXES[1] = (uint64_t)entry;  // hart 1
    HART_MAILBOXES[2] = (uint64_t)entry;  // hart 2
    HART_MAILBOXES[3] = (uint64_t)entry;  // hart 3
    asm volatile("sev");
}

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

static int receive_chunk(uint8_t *dst, uint16_t size) {
    // Receive chunk data
    for (uint16_t i = 0; i < size; i++) {
        dst[i] = (uint8_t)uart_getc();
    }

    // Receive CRC32
    uint32_t received_crc = uart_get32();
    uint32_t computed_crc = crc32(dst, size);

    if (computed_crc != received_crc) {
        uart_putc(NAK);
        return 0;
    }

    uart_putc(ACK);
    return 1;
}

static void receive_payload(uint64_t dtb) {
    uint32_t size;
    uint8_t *dst = LOAD_ADDR;

    while (1) {
        uart_flush_rx();

        uart_putc(READY_BYTE);
        uart_putc(READY_BYTE);
        uart_putc(READY_BYTE);

        volatile uint32_t timeout = 3000000;
        while (timeout > 0 && (*UART0_FR & FR_RXFE)) {
            timeout--;
        }

        if (timeout == 0) continue;

        size = uart_get32();

        if (size == 0 || size > 0x7F80000) {
            uart_puts("error: bad size\r\n");
            continue;
        }

        break;
    }

    uart_putc(ACK_O);
    uart_putc(ACK_K);

    uint32_t received = 0;
    while (received < size) {
        // Compute this chunk's size
        uint32_t remaining = size - received;
        uint16_t chunk_size = remaining > CHUNK_SIZE ? CHUNK_SIZE : (uint16_t)remaining;

        // Attempt chunk with retries
        int success = 0;
        for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
            if (receive_chunk(dst + received, chunk_size)) {
                success = 1;
                break;
            }
            // NAK already sent by receive_chunk, host will retransmit
        }

        if (!success) {
            uart_puts("error: max retries exceeded\r\n");
            return;  // back to ready loop
        }

        received += chunk_size;
    }

    uart_putc(GO_G);
    uart_putc(GO_O);
    uart_puts("info: Transfer OK, jumping to payload\r\n");

    for (volatile int i = 0; i < 10000; i++);

    wake_harts(LOAD_ADDR);

    void (*payload)(uint64_t, uint64_t, uint64_t, uint64_t) = (void (*)(uint64_t, uint64_t, uint64_t, uint64_t))LOAD_ADDR;
    payload(dtb, 0, 0, 0);
}

void main(uint64_t dtb) {
    uart_init();
    uart_puts("info: Wyvern Serial Bootloader for rpi3\r\n");

    /* Loop forever — if payload returns or receive fails,
     * wait for next upload rather than hanging
     */
    while (1) {
        receive_payload(dtb);
    }
}