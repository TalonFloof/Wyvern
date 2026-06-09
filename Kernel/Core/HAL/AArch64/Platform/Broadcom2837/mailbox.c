#include "mailbox.h"
#include "../../../hal.h"

int mbox_call(volatile uint32_t *buf) {
    uint32_t addr = (uint32_t)(uintptr_t)buf;
    
    /* Buffers have to be 16 byte aligned (The low 4 bits are used for the channel) */
    uint32_t msg = (addr & ~0xF) | (0xC0000000) | MBOX_CHANNEL_PM;
    
    hal_sync_fence(); /* Fence to ensure the data is actually written */
    
    // Wait until mailbox not full
    while (MBOX_STATUS & MBOX_FULL);
    MBOX_WRITE = msg;
    
    // Wait for response on our channel
    while (1) {
        while (MBOX_STATUS & MBOX_EMPTY);
        uint32_t resp = MBOX_READ;
        if ((resp & 0xF) == MBOX_CHANNEL_PM && 
            (resp & ~0xF) == (addr & ~0xF))  {
            hal_sync_fence();
            return (buf[1] == 0x80000000);
        }
    }
}