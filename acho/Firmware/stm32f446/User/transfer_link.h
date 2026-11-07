#ifndef TRANSFER_LINK_H
#define TRANSFER_LINK_H
#include "bsp_usart.h"
#include <stddef.h>
#include <stdint.h>
/* Optional RTT trace for the ESP32 USART2 link.  It never writes to the
 * shared physical UART pins; enable it only while diagnosing the link. */
#ifndef TRANSFER_UART_TRACE
#define TRANSFER_UART_TRACE 0
#endif
void Transfer_Init(void);
void Transfer_SetStorageReady(int ready);
void Transfer_Task(void);
void Transfer_PollCallbacks(void);
void Transfer_CancelCallbacks(void);
/* Asynchronous: wait for IsPaused before using the shared programming pins. */
void Transfer_SetPaused(int paused);
int Transfer_IsPaused(void);
enum { TRANSFER_IDLE,TRANSFER_WAIT_BEGIN,TRANSFER_WAIT_DATA,TRANSFER_WAIT_END,TRANSFER_DONE,TRANSFER_PAUSED };
typedef struct {
    uint32_t rx_bytes,rx_frames,uart_errors,received,total;
    uint16_t request_id,last_id;
    uint8_t stage,last_type,tx_status,result;
} TransferStatus;
/* Snapshot for the GUI: diagnostics must not print onto ESP32's shared TX line. */
void Transfer_GetStatus(TransferStatus *status);
int Transfer_Submit(const char *json, const char *path,
                    UartHandle_FinishCallBackFuncTypedef callback, void *userdata);
/* Buffer and received must remain valid until callback. Control callbacks are
 * delivered even when page callbacks are cancelled; callers use stable storage. */
int Transfer_SubmitMemory(const char *json, uint8_t *buffer, size_t capacity, size_t *received,
                          UartHandle_FinishCallBackFuncTypedef callback, void *userdata);
int Transfer_SubmitMemoryWithTimeout(const char *json,uint8_t *buffer,size_t capacity,size_t *received,
                                   uint32_t initial_timeout_ms,UartHandle_FinishCallBackFuncTypedef callback,void *userdata);
#endif
