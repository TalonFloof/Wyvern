#ifndef _WYVERN_HAL_H
#define _WYVERN_HAL_H
#include <stdint.h>

void hal_early_init(void *dtb);

int hal_irq_enable_disable(int enable);

void hal_fence();
void hal_write_fence(); 
void hal_read_fence();
void hal_sync_fence();
void hal_inst_fence();   // Invalidate I-cache, all inner shareable

#endif
