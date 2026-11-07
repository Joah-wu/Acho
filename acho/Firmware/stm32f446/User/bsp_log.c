/* Minimal RTT-compatible output channel, implemented locally for OpenOCD.
 * Wire layout follows OpenOCD's RTT control block/channel reader:
 * https://github.com/openocd-org/openocd/blob/v0.12.0/src/target/rtt.c
 * This is not the SEGGER RTT library. No UART or debugger attachment is needed
 * to write a log; the host reads RAM through the existing SWD connection. */
#include "bsp_log.h"
#include "stm32f4xx.h"
#include <errno.h>
#include <string.h>

enum { LOG_CAPACITY=1024 };
typedef struct {
    const char *name;
    char *buffer;
    uint32_t size;
    volatile uint32_t write_pos,read_pos;
    uint32_t flags;
} LogRttChannel;
typedef struct {
    char id[16];
    uint32_t up_count,down_count;
    LogRttChannel up;
} LogRttControl;

#if UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(LogRttChannel)==24,"RTT channel wire layout");
_Static_assert(offsetof(LogRttControl,up)==24,"RTT control block wire layout");
_Static_assert(offsetof(LogRttChannel,write_pos)==12,"RTT write position layout");
_Static_assert(offsetof(LogRttChannel,read_pos)==16,"RTT read position layout");
#endif

static char log_buffer[LOG_CAPACITY];
static volatile uint32_t dropped_bytes;
/* Standard discovery marker and single up-channel; flag 0 is NO_BLOCK_SKIP. */
LogRttControl _SEGGER_RTT={
    .id="SEGGER RTT",
    .up_count=1,
    .up={.name="Acho log",.buffer=log_buffer,.size=LOG_CAPACITY}
};

size_t DebugLog_Write(const void *data,size_t length) {
    if(!length) return 0;
    if(!data) return 0;
    uint32_t saved=__get_PRIMASK();
    __disable_irq();
    uint32_t write=_SEGGER_RTT.up.write_pos;
    uint32_t read=_SEGGER_RTT.up.read_pos;
    size_t room=read>write?read-write-1:LOG_CAPACITY-write+read-1;
    if(length>room) {
        dropped_bytes+=(uint32_t)length;
        __set_PRIMASK(saved);
        return 0;
    }
    size_t first=LOG_CAPACITY-write;
    if(first>length) first=length;
    memcpy(log_buffer+write,data,first);
    if(first<length) memcpy(log_buffer,(const char*)data+first,length-first);
    __DMB();
    _SEGGER_RTT.up.write_pos=(write+(uint32_t)length)%LOG_CAPACITY;
    __set_PRIMASK(saved);
    return length;
}

uint32_t DebugLog_DroppedBytes(void) { return dropped_bytes; }

/* Override the weak newlib syscall in generated syscalls.c, without editing it.
 * A dropped write is consumed so libc will not retry or block when no host polls. */
int _write(int file,char *data,int length) {
    if(file!=1 && file!=2) {errno=EBADF;return -1;}
    if(length<0 || (!data && length)) {errno=EINVAL;return -1;}
    DebugLog_Write(data,(size_t)length);
    return length;
}
