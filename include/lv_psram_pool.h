#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Backs LVGL's entire built-in allocator arena (LV_MEM_SIZE) with a single
// PSRAM allocation instead of internal DRAM - see lv_conf.h's LV_MEM_POOL_ALLOC.
void* lv_psram_pool_alloc(size_t size);

#ifdef __cplusplus
}
#endif
