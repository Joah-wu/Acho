#ifndef ACHO_BSP_LOG_H
#define ACHO_BSP_LOG_H

#include <stddef.h>
#include <stdint.h>

/* Non-blocking RTT channel 0. A full buffer drops the entire new write. */
size_t DebugLog_Write(const void *data,size_t length);
uint32_t DebugLog_DroppedBytes(void);

#endif
