#include "uart.h"
#include "mailbox.h"

#define GPIO_BASE       (0x3F000000 + 0x200000)
#define GPFSEL1         (*(volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPPUD           (*(volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK0       (*(volatile uint32_t*)(GPIO_BASE + 0x98))

#define UART0_BASE      (0x3F000000 + 0x201000)
#define UART0_DR        (*(volatile uint32_t*)(UART0_BASE + 0x00))
#define UART0_FR        (*(volatile uint32_t*)(UART0_BASE + 0x18))
#define UART0_IBRD      (*(volatile uint32_t*)(UART0_BASE + 0x24))
#define UART0_FBRD      (*(volatile uint32_t*)(UART0_BASE + 0x28))
#define UART0_LCRH      (*(volatile uint32_t*)(UART0_BASE + 0x2C))
#define UART0_CR        (*(volatile uint32_t*)(UART0_BASE + 0x30))
#define UART0_IMSC      (*(volatile uint32_t*)(UART0_BASE + 0x38))
#define UART0_ICR       (*(volatile uint32_t*)(UART0_BASE + 0x44))

#define FR_BUSY         (1u << 3)
#define FR_RXFE         (1u << 4)   /* RX FIFO empty */
#define FR_TXFF         (1u << 5)   /* TX FIFO full */
#define LCRH_FEN        (1u << 4)   /* FIFO enable */
#define LCRH_WLEN_8     (3u << 5)   /* 8-bit words */
#define CR_UARTEN       (1u << 0)
#define CR_TXE          (1u << 8)
#define CR_RXE          (1u << 9)

static void gpio_uart_init() {
    uint32_t sel = GPFSEL1;
    sel &= ~((7u << 12) | (7u << 15));  /* clear both fields */
    sel |=  ((4u << 12) | (4u << 15));  /* set ALT0 */
    GPFSEL1 = sel;

    /* Disable pull-up/down on GPIO14 and GPIO15 */
    GPPUD = 0; /* 1. No pull */
    delay_cycles(150); /* 2. Wait 150 cycles */
    GPPUDCLK0 = (1u << 14) | (1u << 15); /* 3. Assert clock to target pins */
    delay_cycles(150); /* 4. Wait 150 cycles */
    GPPUD = 0; /* 5. Clear GPPUD */
    GPPUDCLK0 = 0; /* 6. Release clock */
}

void uart_init() {
    volatile uint32_t __attribute__((aligned(16))) mbox_buf[9] = {
        9 * 4,                   /* [0] buffer size in bytes */
        0x00000000,              /* [1] request code */
        MBOX_TAG_SET_CLOCK_RATE, /* [2] tag: set clock rate */
        12,                      /* [3] value buffer size in bytes */
        0,                       /* [4] request indicator */
        0x00000002,              /* [5] clock ID: UART */
        4000000,                 /* [6] rate: 4MHz */
        0,                       /* [7] skip turbo */
        0x00000000               /* [8] end tag */
    };

    /* Disable UART */
    UART0_CR = 0;
    // Set clock to 4MHz via VideoCore mailbox */
    mbox_call(mbox_buf);
    gpio_uart_init();
    /* Flush any transmissions */
    while (UART0_FR & FR_BUSY);
    UART0_LCRH &= ~LCRH_FEN;
    /* Set baud to 115200 */
    UART0_IBRD = 2;
    UART0_FBRD = 11;
    /* Set format to 8N1 and enable the FIFO queue */
    UART0_LCRH = LCRH_WLEN_8 | LCRH_FEN;
    UART0_ICR  = 0x7FF;
    UART0_IMSC = 0;
    /* FINALLY Enable UART, TX, RX */
    UART0_CR = CR_UARTEN | CR_TXE | CR_RXE;
}

void uart_putc(char c) {
    while (UART0_FR & FR_TXFF);
    UART0_DR = (uint32_t)c;
}

char uart_getc(void) {
    while (UART0_FR & FR_RXFE);
    return (char)(UART0_DR & 0xFF);
}