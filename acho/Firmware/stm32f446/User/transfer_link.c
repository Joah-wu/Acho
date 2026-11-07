/* USART2 owns the ESP32 link; shared USART1 TX must remain high-Z. */
#include "transfer_link.h"
#include "acho_protocol.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

#define RING_SIZE 2048u
#define QUEUE_DEPTH 3
#define PATH_SIZE 180
#define REQUEST_SIZE 256
typedef struct {
    char json[REQUEST_SIZE], path[PATH_SIZE];
    UartHandle_FinishCallBackFuncTypedef callback;
    void *userdata;
    uint32_t generation;
    uint8_t *memory;
    size_t capacity;
    size_t *received;
    uint32_t initial_timeout_ms;
} request_t;
typedef struct {
    UartHandle_FinishCallBackFuncTypedef callback;
    void *userdata;
    HAL_StatusTypeDef status;
    uint32_t generation;
} completion_t;
static QueueHandle_t requests, completions;
static StaticQueue_t request_control, completion_control;
static uint8_t request_memory[QUEUE_DEPTH*sizeof(request_t)];
static uint8_t completion_memory[(QUEUE_DEPTH+1)*sizeof(completion_t)];
static uint8_t ring[RING_SIZE], irq_byte;
static volatile uint16_t head, tail;
static volatile uint8_t overflow;
static volatile uint32_t generation;
static volatile int storage_ready;
static volatile int pause_requested, paused;
static volatile TransferStatus link_status;
static acho_parser_t parser;
static acho_frame_t frame, outgoing;
static uint8_t wire[ACHO_HEADER+ACHO_CHUNK];
static uint16_t next_id, last_end_id, last_end_seq;
static TickType_t last_end_time;
static FIL output;

#if TRANSFER_UART_TRACE
/* Called only by the transfer task, never by the UART interrupt.  Text is
 * printed as text; protocol control bytes use \xNN so the full stream stays
 * readable without sending anything on USART2. */
static const char *trace_direction;
static char trace_line[108];
static size_t trace_used;
static void trace_uart_flush(void) {
    if(trace_direction && trace_used) {
        trace_line[trace_used]=0;
        Printf("UART2 %s: %s\r\n",trace_direction,trace_line);
        trace_used=0;
    }
}
static void trace_uart_bytes(const char *direction,const uint8_t *bytes,size_t length) {
    static const char hex[]="0123456789ABCDEF";
    if(!trace_direction || strcmp(trace_direction,direction)) {
        trace_uart_flush();
        trace_direction=direction;
    }
    while(length--) {
        uint8_t byte=*bytes++;
        if(byte=='\r') continue; /* Normalize ESP32 CRLF into one RTT line. */
        if(byte=='\n') { trace_uart_flush(); continue; }
        size_t needed=(byte>=32u && byte<=126u)?1u:4u;
        if(trace_used+needed>=sizeof(trace_line)) trace_uart_flush();
        if(byte>=32u && byte<=126u) trace_line[trace_used++]=(char)byte;
        else {
            trace_line[trace_used++]='\\';trace_line[trace_used++]='x';
            trace_line[trace_used++]=hex[byte>>4];trace_line[trace_used++]=hex[byte&15u];
        }
    }
}
#endif

void Transfer_Init(void) {
    /* PA9 and ESP32 TX share CH549 RX even outside programming mode. */
    DebugUart_SetEnabled(0);
    requests=xQueueCreateStatic(QUEUE_DEPTH,sizeof(request_t),request_memory,&request_control);
    completions=xQueueCreateStatic(QUEUE_DEPTH+1,sizeof(completion_t),completion_memory,&completion_control);
    configASSERT(requests && completions);
    HAL_NVIC_SetPriority(USART2_IRQn,5,0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_StatusTypeDef started=HAL_UART_Receive_IT(&huart2,&irq_byte,1);
    configASSERT(started==HAL_OK);
}
void Transfer_SetStorageReady(int ready) { storage_ready=ready; }
void Transfer_SetPaused(int value) { pause_requested=value!=0; }
int Transfer_IsPaused(void) { return paused; }
void Transfer_GetStatus(TransferStatus *status) {
    taskENTER_CRITICAL();*status=link_status;taskEXIT_CRITICAL();
}

/* Only the USART2 owner changes its peripheral/pins, after download cleanup. */
static void set_paused(int value) {
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_2|GPIO_PIN_3;
    gpio.Pull=GPIO_NOPULL;
    gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    HAL_UART_AbortReceive(&huart2);
    __HAL_UART_DISABLE(&huart2);
    head=tail=0;overflow=0;parser.used=0;last_end_id=0;
    if(value) {
        gpio.Mode=GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOA,&gpio);
        paused=1;
        link_status.stage=TRANSFER_PAUSED;
        link_status.result=HAL_BUSY;
    } else {
        gpio.Mode=GPIO_MODE_AF_PP;
        gpio.Alternate=GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA,&gpio);
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        __HAL_UART_ENABLE(&huart2);
        paused=0;
        link_status=(TransferStatus){.stage=TRANSFER_IDLE};
        HAL_StatusTypeDef started=HAL_UART_Receive_IT(&huart2,&irq_byte,1);
        configASSERT(started==HAL_OK);
        HAL_NVIC_ClearPendingIRQ(USART2_IRQn);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    if(uart!=&huart2 || pause_requested || paused) return;
    link_status.rx_bytes++;
    uint16_t next=(head+1u)%RING_SIZE;
    if(next==tail) overflow=1;
    else { ring[head]=irq_byte; __DMB(); head=next; }
    HAL_UART_Receive_IT(&huart2,&irq_byte,1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
    if(uart!=&huart2 || pause_requested || paused) return;
    link_status.uart_errors++;
    overflow=1;
    HAL_UART_AbortReceive_IT(&huart2);
    HAL_UART_Receive_IT(&huart2,&irq_byte,1);
}
static void send_frame(uint8_t type, uint16_t id, uint16_t seq, const void *data, uint16_t len) {
    if(pause_requested || paused) return;
    outgoing.type=type; outgoing.id=id; outgoing.seq=seq; outgoing.len=len;
    if(len) memcpy(outgoing.data,data,len);
    size_t n=acho_encode(wire,&outgoing);
    link_status.tx_status=HAL_UART_Transmit(&huart2,wire,n,1000);
#if TRANSFER_UART_TRACE
    trace_uart_bytes("TX",wire,n);
    trace_uart_flush();
#endif
}
static int receive_frame(TickType_t timeout) {
#if TRANSFER_UART_TRACE
    uint8_t trace[16];
    size_t trace_used=0;
#endif
    TickType_t start=xTaskGetTickCount();
    do {
        if(pause_requested) return -1;
        if(overflow) {
            taskENTER_CRITICAL(); tail=head; overflow=0; taskEXIT_CRITICAL();
            parser.used=0;
            return -1;
        }
        while(tail!=head) {
            uint8_t byte=ring[tail];
            __DMB(); tail=(tail+1u)%RING_SIZE;
#if TRANSFER_UART_TRACE
            trace[trace_used++]=byte;
            if(trace_used==sizeof(trace)) {
                trace_uart_bytes("RX",trace,trace_used);
                trace_used=0;
            }
#endif
            if(acho_feed(&parser,byte,&frame)) {
#if TRANSFER_UART_TRACE
                if(trace_used) trace_uart_bytes("RX",trace,trace_used);
#endif
                link_status.rx_frames++;
                link_status.last_type=frame.type;link_status.last_id=frame.id;
                return 1;
            }
        }
#if TRANSFER_UART_TRACE
        if(trace_used) {
            trace_uart_bytes("RX",trace,trace_used);
            trace_used=0;
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(1));
    } while(xTaskGetTickCount()-start<timeout);
    return 0;
}
static int old_end(void) {
    if(frame.type==ACHO_END && frame.id==last_end_id && frame.seq==last_end_seq &&
       xTaskGetTickCount()-last_end_time<pdMS_TO_TICKS(10000)) {
        send_frame(ACHO_ACK,frame.id,frame.seq,NULL,0);
        return 1;
    }
    return 0;
}
/* FatFs rename cannot replace a destination. Keep a backup until installation
 * succeeds, and restore it if the second rename fails. */
static FRESULT install_file(const char *temporary, const char *path) {
    char backup[PATH_SIZE+5];
    snprintf(backup,sizeof(backup),"%s.bak",path);
    FILINFO info;
    FRESULT exists=f_stat(path,&info);
    if(exists!=FR_OK && exists!=FR_NO_FILE) return exists;
    if(exists==FR_OK) {
        FRESULT r=f_unlink(backup);
        if(r!=FR_OK && r!=FR_NO_FILE) return r;
        r=f_rename(path,backup);
        if(r!=FR_OK) return r;
    }
    FRESULT r=f_rename(temporary,path);
    if(r!=FR_OK && exists==FR_OK) f_rename(backup,path);
    if(r==FR_OK && exists==FR_OK) f_unlink(backup);
    return r;
}
static HAL_StatusTypeDef download(const request_t *request) {
    if(request->received) *request->received=0;
    if(pause_requested || paused) return HAL_BUSY;
    if(!request->memory && !storage_ready) return HAL_ERROR;
    char temporary[PATH_SIZE+5]={0};
    if(!request->memory) {
        snprintf(temporary,sizeof(temporary),"%s.part",request->path);
        char directory[PATH_SIZE];
        strcpy(directory,request->path);
        char *slash=strrchr(directory,'/');
        if(slash) {
            *slash=0;
            FRESULT r=f_mkdir(directory);
            if(r!=FR_OK && r!=FR_EXIST) return HAL_ERROR;
        }
    }
    uint16_t id=++next_id;
    if(id==0) id=++next_id;
    taskENTER_CRITICAL();
    link_status=(TransferStatus){.request_id=id,.stage=TRANSFER_WAIT_BEGIN,.result=HAL_BUSY};
    taskEXIT_CRITICAL();
    uint16_t expected=0;
    uint32_t total=0, received=0, crc=0;
    int opened=0, begun=0;
    TickType_t activity=xTaskGetTickCount();
    send_frame(ACHO_REQUEST,id,0,request->json,strlen(request->json));
    HAL_StatusTypeDef result=HAL_TIMEOUT;
    TickType_t last_request=activity;
    unsigned request_attempts=1;
    for(;;) {
        int got=receive_frame(pdMS_TO_TICKS(50));
        if(got<0) { result=pause_requested?HAL_BUSY:HAL_ERROR; break; }
        if(xTaskGetTickCount()-activity>pdMS_TO_TICKS(request->memory?(begun?10000:(request->initial_timeout_ms?request->initial_timeout_ms:10000)):(begun?30000:120000))) break;
        /* ESP32 may still be booting when STM32 sends its first status query. */
        if(!begun && request->memory && request->initial_timeout_ms<=10000 && request_attempts<3 &&
           xTaskGetTickCount()-last_request>=pdMS_TO_TICKS(1000)) {
            send_frame(ACHO_REQUEST,id,0,request->json,strlen(request->json));
            last_request=xTaskGetTickCount(); request_attempts++;
        }
        if(!got || old_end() || frame.id!=id) continue;
        if(frame.type==ACHO_ERROR) { result=HAL_ERROR; break; }
        if(frame.type!=ACHO_BEGIN && frame.type!=ACHO_DATA && frame.type!=ACHO_END) continue;
        /* Lost ACK: repeat the ACK without writing the block twice. */
        if(begun && frame.seq==(uint16_t)(expected-1)) {
            send_frame(ACHO_ACK,id,frame.seq,NULL,0);
            continue;
        }
        if(frame.seq!=expected) continue;
        activity=xTaskGetTickCount();
        if(frame.type==ACHO_BEGIN && !begun && frame.len==4) {
            total=acho_u32(frame.data);
            if(total>ACHO_MAX_FILE || (request->memory && total>request->capacity)) {
                result=HAL_ERROR; break;
            }
            if(!request->memory) {
                if(f_open(&output,temporary,FA_CREATE_ALWAYS|FA_WRITE)!=FR_OK) { result=HAL_ERROR; break; }
                opened=1;
            }
            begun=1;
            link_status.total=total;
            link_status.stage=total?TRANSFER_WAIT_DATA:TRANSFER_WAIT_END;
        } else if(frame.type==ACHO_DATA && begun && frame.len && frame.len<=total-received) {
            if(request->memory) memcpy(request->memory+received,frame.data,frame.len);
            else {
                UINT written=0;
                if(f_write(&output,frame.data,frame.len,&written)!=FR_OK || written!=frame.len) {
                    result=HAL_ERROR; break;
                }
            }
            crc=acho_crc32(crc,frame.data,frame.len);
            received+=frame.len;
            link_status.received=received;
            if(received==total) link_status.stage=TRANSFER_WAIT_END;
        } else if(frame.type==ACHO_END && begun && frame.len==8 && received==total &&
                  acho_u32(frame.data)==total && acho_u32(frame.data+4)==crc) {
            if(!request->memory) {
                FRESULT closed=f_close(&output);
                opened=0;
                if(closed!=FR_OK || install_file(temporary,request->path)!=FR_OK) { result=HAL_ERROR; break; }
            }
            if(request->received) *request->received=received;
            last_end_id=id; last_end_seq=frame.seq; last_end_time=xTaskGetTickCount();
            send_frame(ACHO_ACK,id,frame.seq,NULL,0);
            link_status.stage=TRANSFER_DONE;link_status.result=HAL_OK;
            return HAL_OK;
        } else { result=HAL_ERROR; break; }
        send_frame(ACHO_ACK,id,frame.seq,NULL,0);
        expected++;
    }
    send_frame(ACHO_ERROR,id,expected,NULL,0);
    if(opened) f_close(&output);
    if(!request->memory) f_unlink(temporary);
    link_status.result=result;
    return result;
}
int Transfer_Submit(const char *json, const char *path,
                    UartHandle_FinishCallBackFuncTypedef callback, void *userdata) {
    if(pause_requested || paused || !requests || !json || !path || strlen(json)>=REQUEST_SIZE || strlen(path)>=PATH_SIZE) return 0;
    request_t request={.callback=callback,.userdata=userdata,.generation=generation};
    strcpy(request.json,json); strcpy(request.path,path);
    return xQueueSend(requests,&request,0)==pdTRUE;
}
int Transfer_SubmitMemory(const char *json, uint8_t *buffer, size_t capacity, size_t *received,
                          UartHandle_FinishCallBackFuncTypedef callback, void *userdata) {
    return Transfer_SubmitMemoryWithTimeout(json,buffer,capacity,received,10000,callback,userdata);
}
int Transfer_SubmitMemoryWithTimeout(const char *json,uint8_t *buffer,size_t capacity,size_t *received,
                                   uint32_t initial_timeout_ms,UartHandle_FinishCallBackFuncTypedef callback,void *userdata) {
    if(pause_requested || paused || !requests || !json || !buffer || !capacity || !received || !initial_timeout_ms ||
       initial_timeout_ms>120000 || strlen(json)>=REQUEST_SIZE) return 0;
    request_t request={.callback=callback,.userdata=userdata,.generation=UINT32_MAX,
        .memory=buffer,.capacity=capacity,.received=received,.initial_timeout_ms=initial_timeout_ms};
    strcpy(request.json,json);
    return xQueueSend(requests,&request,0)==pdTRUE;
}
void Transfer_CancelCallbacks(void) { generation++; }
void Transfer_PollCallbacks(void) {
    completion_t completion;
    while(completions && xQueueReceive(completions,&completion,0)==pdTRUE)
        if(completion.callback && (completion.generation==UINT32_MAX || completion.generation==generation))
            completion.callback(completion.status,completion.userdata);
}
void Transfer_Task(void) {
    static request_t request;
    for(;;) {
        if(pause_requested && !paused) set_paused(1);
        else if(!pause_requested && paused) set_paused(0);
        if(xQueueReceive(requests,&request,pdMS_TO_TICKS(20))==pdTRUE) {
            HAL_StatusTypeDef status=download(&request);
            if(request.callback) {
                completion_t completion={request.callback,request.userdata,status,request.generation};
                xQueueSend(completions,&completion,portMAX_DELAY);
            }
        } else if(!paused && receive_frame(pdMS_TO_TICKS(1))>0) old_end();
    }
}
