#ifndef _BROADCOM2837_MAILBOX_H
#define _BROADCOM2837_MAILBOX_H
#include <stdint.h>

#define MBOX_BASE       0x3F00B880
#define MBOX_READ       (*(volatile uint32_t*)(MBOX_BASE + 0x00))
#define MBOX_STATUS     (*(volatile uint32_t*)(MBOX_BASE + 0x18))
#define MBOX_WRITE      (*(volatile uint32_t*)(MBOX_BASE + 0x20))
#define MBOX_FULL       (1u << 31)
#define MBOX_EMPTY      (1u << 30)
#define MBOX_CHANNEL_PM 8

#define MBOX_TAG_SET_CLOCK_RATE 0x38002

int mbox_call(volatile uint32_t *buf);
#endif