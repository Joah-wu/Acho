/* Exercise the real non-blocking producer and a simulated SWD consumer. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
uint32_t test_log_primask;
#include "../../stm32f446/User/bsp_log.c"

static size_t drain(char *out) {
    size_t count=0;
    while(_SEGGER_RTT.up.read_pos!=_SEGGER_RTT.up.write_pos) {
        uint32_t at=_SEGGER_RTT.up.read_pos;
        out[count++]=_SEGGER_RTT.up.buffer[at];
        _SEGGER_RTT.up.read_pos=(at+1)%LOG_CAPACITY;
    }
    return count;
}
int main(void) {
    char input[LOG_CAPACITY],output[LOG_CAPACITY];
    memset(input,'x',sizeof(input));
    assert(!strcmp(_SEGGER_RTT.id,"SEGGER RTT") && _SEGGER_RTT.up_count==1 && !_SEGGER_RTT.down_count);
    assert(DebugLog_Write("boot\n",5)==5 && !test_log_primask);
    assert(drain(output)==5 && !memcmp(output,"boot\n",5));
    /* Preserve the caller's interrupt state, including use before the scheduler. */
    test_log_primask=1;
    assert(DebugLog_Write("locked",6)==6 && test_log_primask==1);
    test_log_primask=0;assert(drain(output)==6);
    for(unsigned i=0;i<1000;i++) {
        for(unsigned j=0;j<77;j++) input[j]=(char)(i+j);
        assert(DebugLog_Write(input,77)==77);
        assert(drain(output)==77 && !memcmp(input,output,77));
    }
    memset(input,'q',sizeof(input));
    assert(DebugLog_Write(input,LOG_CAPACITY-1)==LOG_CAPACITY-1);
    uint32_t at=_SEGGER_RTT.up.write_pos;
    assert(!DebugLog_Write("drop",4) && DebugLog_DroppedBytes()==4 && _SEGGER_RTT.up.write_pos==at);
    assert(_write(1,"lost",4)==4 && DebugLog_DroppedBytes()==8);
    assert(drain(output)==LOG_CAPACITY-1 && !memcmp(input,output,LOG_CAPACITY-1));
    assert(_write(2,"stderr\n",7)==7 && drain(output)==7 && !memcmp(output,"stderr\n",7));
    assert(!DebugLog_Write(NULL,0) && !DebugLog_Write(NULL,5));
    assert(!DebugLog_Write(input,LOG_CAPACITY) && DebugLog_DroppedBytes()==8+LOG_CAPACITY);
    assert(_write(3,input,1)==-1 && errno==EBADF);
    assert(_write(1,NULL,1)==-1 && errno==EINVAL);
    assert(_write(1,input,-1)==-1 && errno==EINVAL);
    assert(DebugLog_Write("resume\n",7)==7 && drain(output)==7 && !test_log_primask);
    puts("RTT: discovery, preserved IRQ state, wrap, full/oversize drop, stdout/stderr and resumed output passed");
}
