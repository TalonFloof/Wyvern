#include <stdint.h>

/* BCM2837 peripheral base */
#define MMIO_BASE       0x3F000000

/* GPIO registers */
#define GPFSEL1         ((volatile uint32_t *)(MMIO_BASE + 0x200004))
#define GPPUD           ((volatile uint32_t *)(MMIO_BASE + 0x200094))
#define GPPUDCLK0       ((volatile uint32_t *)(MMIO_BASE + 0x200098))

/* Auxiliary peripheral registers */
#define AUX_ENABLES     ((volatile uint32_t *)(MMIO_BASE + 0x215004))
#define AUX_MU_IO       ((volatile uint32_t *)(MMIO_BASE + 0x215040))
#define AUX_MU_IER      ((volatile uint32_t *)(MMIO_BASE + 0x215044))
#define AUX_MU_IIR      ((volatile uint32_t *)(MMIO_BASE + 0x215048))
#define AUX_MU_LCR      ((volatile uint32_t *)(MMIO_BASE + 0x21504C))
#define AUX_MU_MCR      ((volatile uint32_t *)(MMIO_BASE + 0x215050))
#define AUX_MU_LSR      ((volatile uint32_t *)(MMIO_BASE + 0x215054))
#define AUX_MU_CNTL     ((volatile uint32_t *)(MMIO_BASE + 0x215060))
#define AUX_MU_BAUD     ((volatile uint32_t *)(MMIO_BASE + 0x215068))

/* AUX_MU_LSR status bits */
#define AUX_MU_LSR_TX_EMPTY  (1 << 5)
#define AUX_MU_LSR_RX_READY  (1 << 0)

static void delay(int32_t count) {
    /* Simple busy loop for timing sensitive GPIO operations */
    asm volatile("__delay_%=: subs %[count], %[count], #1\n"
                 "bne __delay_%=\n"
                 : [count] "+r" (count));
}

void uart_init(void) {
    /* Enable Mini UART in the auxiliary block */
    *AUX_ENABLES = 1;

    /* Disable TX/RX while configuring */
    *AUX_MU_CNTL = 0;

    /* Disable interrupts */
    *AUX_MU_IER = 0;

    /* Set 8-bit mode */
    *AUX_MU_LCR = 3;

    /* Set RTS line high */
    *AUX_MU_MCR = 0;

    /* Clear TX/RX FIFOs */
    *AUX_MU_IIR = 0xC6;

    /* Set baud rate to 115200
     * Baud rate register = (CPU clock / (8 * baud)) - 1
     * BCM2837 core clock is 250MHz
     * (250000000 / (8 * 115200)) - 1 = 270
     */
    *AUX_MU_BAUD = 270;

    /* Configure GPIO14 and GPIO15 for Alt5 (Mini UART)
     * GPFSEL1 controls GPIO 10-19, 3 bits per pin
     * GPIO14 is bits 14:12, GPIO15 is bits 17:15
     * Alt5 = 0b010
     */
    uint32_t selector = *GPFSEL1;
    selector &= ~(7 << 12);   /* clear GPIO14 */
    selector |=  (2 << 12);   /* set Alt5 for GPIO14 */
    selector &= ~(7 << 15);   /* clear GPIO15 */
    selector |=  (2 << 15);   /* set Alt5 for GPIO15 */
    *GPFSEL1 = selector;

    /* Disable pull up/down for GPIO14 and GPIO15 */
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUD = 0;
    *GPPUDCLK0 = 0;

    /* Enable TX and RX */
    *AUX_MU_CNTL = 3;
}

void uart_putc(char c) {
    /* Wait until transmit FIFO has space */
    while (!(*AUX_MU_LSR & AUX_MU_LSR_TX_EMPTY));
    *AUX_MU_IO = c;
}

char uart_getc(void) {
    /* Wait until a byte is available */
    while (!(*AUX_MU_LSR & AUX_MU_LSR_RX_READY));
    return (char)(*AUX_MU_IO & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}