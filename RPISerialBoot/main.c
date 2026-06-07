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

static int receive_chunk(uint8_t *dst, uint16_t size, uint32_t chunk_num) {
    // Receive chunk data with timeout
    for (uint16_t i = 0; i < size; i++) {
        int b = uart_getc_timeout(1000);
        if (b < 0) return -1;
        dst[i] = (uint8_t)b;
    }

    // Receive CRC32 with timeout
    uint32_t received_crc = 0;
    for (int i = 0; i < 4; i++) {
        int b = uart_getc_timeout(1000);
        if (b < 0) return -1;
        received_crc |= (uint32_t)b << (i * 8);
    }

    uint32_t computed_crc = crc32(dst, size);
    if (computed_crc != received_crc) {
        // NAK with chunk number so host knows which chunk to retransmit
        uart_flush_rx();
        uart_putc(NAK);
        uart_putc((chunk_num)       & 0xFF);
        uart_putc((chunk_num >> 8)  & 0xFF);
        uart_putc((chunk_num >> 16) & 0xFF);
        uart_putc((chunk_num >> 24) & 0xFF);
        return 0;
    }

    // ACK with chunk number so host can track progress
    uart_putc(ACK);
    uart_putc((chunk_num)       & 0xFF);
    uart_putc((chunk_num >> 8)  & 0xFF);
    uart_putc((chunk_num >> 16) & 0xFF);
    uart_putc((chunk_num >> 24) & 0xFF);
    return 1;
}

static void receive_payload(uint64_t dtb) {
    uint8_t *dst = LOAD_ADDR;
    static uint32_t total_size = 0;
    static uint32_t last_good_chunk = 0;
    static int resuming = 0;

    while (1) {
        uart_flush_rx();

        if (!resuming) {
            uart_putc(READY_BYTE);
            uart_putc(READY_BYTE);
            uart_putc(READY_BYTE);
        } else {
            uart_putc('R');
            uart_putc('E');
            uart_putc('S');
            uart_putc((last_good_chunk)       & 0xFF);
            uart_putc((last_good_chunk >> 8)  & 0xFF);
            uart_putc((last_good_chunk >> 16) & 0xFF);
            uart_putc((last_good_chunk >> 24) & 0xFF);
        }

        volatile uint32_t timeout = 3000000;
        while (timeout > 0 && (*UART0_FR & FR_RXFE))
            timeout--;

        if (timeout == 0) continue;

        if (!resuming) {
            total_size = uart_get32();
            if (total_size == 0 || total_size > 0x7F80000) {
                uart_puts("error: bad size\r\n");
                continue;
            }
            uart_putc(ACK_O);
            uart_putc(ACK_K);
            last_good_chunk = 0;
        } else {
            int r1 = uart_getc_timeout(1000);
            if (r1 != 'O') continue;
            int r2 = uart_getc_timeout(1000);
            if (r2 != 'K') continue;
        }
        break;
    }

    uint32_t received = resuming ?
        (uint32_t)(last_good_chunk + 1) * CHUNK_SIZE : 0;
    if (received > total_size) received = total_size;
    uint32_t chunk_num = resuming ? (last_good_chunk + 1) : 0;
    resuming = 0;

    while (received < total_size) {
        uint32_t remaining = total_size - received;
        uint16_t chunk_size = remaining > CHUNK_SIZE ?
                              CHUNK_SIZE : (uint16_t)remaining;

        int success = 0;
        for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
            int result = receive_chunk(dst + received,
                                       chunk_size, chunk_num);
            if (result == 1) {
                last_good_chunk = chunk_num;
                success = 1;
                break;
            } else if (result == 0) {
                continue;   // NAK already sent
            } else {
                // Connection lost
                resuming = 1;
                for (volatile int i = 0; i < 100000; i++);
                uart_flush_rx();
                goto retry;
            }
        }

        if (!success) {
            // Reset static state for next call
            resuming = 0;
            last_good_chunk = 0;
            total_size = 0;
            return;
        }

        received += chunk_size;
        chunk_num++;
    }

    // Success — reset static state before jumping
    resuming = 0;
    last_good_chunk = 0;
    total_size = 0;

    uart_putc(GO_G);
    uart_putc(GO_O);
    uart_puts("info: transfer complete, CRC32 OK\r\n");
    for (volatile int i = 0; i < 10000; i++);

    wake_harts(LOAD_ADDR);
    void (*payload)(uint64_t, uint64_t, uint64_t, uint64_t) =
        (void (*)(uint64_t, uint64_t, uint64_t, uint64_t))LOAD_ADDR;
    payload(dtb, 0, 0, 0);
    return;

retry:
    receive_payload(dtb);
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