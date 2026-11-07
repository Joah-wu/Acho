#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/* Suppress the board BSP header while compiling the real transfer task. */
#define __BSP_USART_H__
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { int unused; } UART_HandleTypeDef;
typedef void (*UartHandle_FinishCallBackFuncTypedef)(HAL_StatusTypeDef,void*);
UART_HandleTypeDef huart2;
static int debug_enabled=1;
void DebugUart_SetEnabled(int value) { debug_enabled=value; }
/* The real target sends optional USART2 traces to RTT through Printf. */
int Printf(char *format,...) { (void)format;return 0; }
typedef struct {uint32_t Pin,Mode,Pull,Speed,Alternate;} GPIO_InitTypeDef;
static unsigned gpio_mode,uart_enabled=1,irq_enabled=1,receive_starts;
#define GPIO_PIN_2 4u
#define GPIO_PIN_3 8u
#define GPIO_NOPULL 0u
#define GPIO_SPEED_FREQ_VERY_HIGH 3u
#define GPIO_MODE_INPUT 0u
#define GPIO_MODE_AF_PP 2u
#define GPIO_AF7_USART2 7u
#define GPIOA ((void*)1)
static void HAL_GPIO_Init(void *port,GPIO_InitTypeDef *gpio) {
    assert(port==GPIOA && gpio->Pin==(GPIO_PIN_2|GPIO_PIN_3) && gpio->Pull==GPIO_NOPULL);
    gpio_mode=gpio->Mode;
}
#define __HAL_UART_DISABLE(u) ((void)(u),uart_enabled=0)
#define __HAL_UART_ENABLE(u) ((void)(u),uart_enabled=1)
#define __HAL_UART_CLEAR_OREFLAG(u) ((void)(u))
#define USART2_IRQn 38
#define __DMB() ((void)0)
#define HAL_NVIC_SetPriority(a,b,c) ((void)0)
#define HAL_NVIC_EnableIRQ(a) ((void)(a),irq_enabled=1)
#define HAL_NVIC_DisableIRQ(a) ((void)(a),irq_enabled=0)
#define HAL_NVIC_ClearPendingIRQ(a) ((void)(a))
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,uint8_t*,uint16_t,uint32_t);
#include "../../stm32f446/User/transfer_link.c"

static TickType_t ticks;
static uint8_t payload[1800];
static size_t source_size, position;
static uint16_t active_id, peer_seq;
static int duplicate_data, write_failure, install_failure, bad_crc, no_response, ack_count, drop_requests,request_count;
static int pause_after_begin;
TickType_t xTaskGetTickCount(void) { return ticks; }
void vTaskDelay(TickType_t n) { ticks+=n; }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *u,uint8_t *b,uint16_t n) {
    (void)u;(void)b;(void)n;receive_starts++;return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef *u) { (void)u;return HAL_OK; }
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *u) { (void)u;return HAL_OK; }
static void inject(uint8_t type,uint16_t seq,const void *data,uint16_t len) {
    acho_frame_t peer={.type=type,.id=active_id,.seq=seq,.len=len};
    if(len) memcpy(peer.data,data,len);
    uint8_t buffer[ACHO_HEADER+ACHO_CHUNK];
    size_t n=acho_encode(buffer,&peer);
    for(size_t i=0;i<n;i++) { irq_byte=buffer[i];HAL_UART_RxCpltCallback(&huart2); }
}
static void peer_next(void) {
    if(position<source_size) {
        size_t n=source_size-position;
        if(n>ACHO_CHUNK) n=ACHO_CHUNK;
        inject(ACHO_DATA,++peer_seq,payload+position,n);
        position+=n;
    } else {
        uint8_t final[8];
        acho_put32(final,source_size);
        acho_put32(final+4,acho_crc32(0,payload,source_size) ^ (bad_crc?1u:0));
        inject(ACHO_END,++peer_seq,final,sizeof(final));
    }
}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u,uint8_t *data,uint16_t n,uint32_t timeout) {
    (void)u;(void)timeout;
    acho_parser_t sent_parser={0};acho_frame_t sent;
    int complete=0;
    for(uint16_t i=0;i<n;i++) complete+=acho_feed(&sent_parser,data[i],&sent);
    assert(complete==1);
    if(sent.type==ACHO_REQUEST) {
        request_count++;
        active_id=sent.id;peer_seq=0;position=0;
        if(drop_requests) { drop_requests--; return HAL_OK; }
        if(!no_response) {
            uint8_t total[4];acho_put32(total,source_size);
            inject(ACHO_BEGIN,0,total,4);
        }
    } else if(sent.type==ACHO_ACK) {
        ack_count++;
        if(sent.seq==1 && duplicate_data && position>0) {
            duplicate_data=0;
            inject(ACHO_DATA,1,payload,position);
        } else if(sent.seq==peer_seq && (sent.seq==0 || position<source_size)) peer_next();
        else if(sent.seq==peer_seq && position==source_size && sent.seq==(source_size?2:0)) peer_next();
        if(pause_after_begin && sent.seq==0) Transfer_SetPaused(1);
    }
    return HAL_OK;
}
FRESULT f_stat(const char *path,FILINFO *info) {
    (void)info;struct stat s;return stat(path,&s)==0?FR_OK:FR_NO_FILE;
}
FRESULT f_unlink(const char *path) { return unlink(path)==0?FR_OK:errno==ENOENT?FR_NO_FILE:FR_DISK_ERR; }
FRESULT f_rename(const char *from,const char *to) {
    if(install_failure && strstr(from,".part")) { install_failure=0;return FR_DISK_ERR; }
    return rename(from,to)==0?FR_OK:FR_DISK_ERR;
}
FRESULT f_mkdir(const char *path) { return mkdir(path,0700)==0?FR_OK:errno==EEXIST?FR_EXIST:FR_DISK_ERR; }
FRESULT f_open(FIL *file,const char *path,int mode) {
    (void)mode;file->file=fopen(path,"wb");return file->file?FR_OK:FR_DISK_ERR;
}
FRESULT f_write(FIL *file,const void *data,UINT size,UINT *written) {
    if(write_failure) { *written=0;return FR_DISK_ERR; }
    *written=fwrite(data,1,size,file->file);return *written==size?FR_OK:FR_DISK_ERR;
}
FRESULT f_close(FIL *file) { return fclose(file->file)==0?FR_OK:FR_DISK_ERR; }
static void original_file(void) {
    FILE *f=fopen("sample.wav","wb");assert(f);fwrite("original",1,8,f);fclose(f);
}
static void check_file(const void *expected,size_t size) {
    uint8_t data[2048];FILE *f=fopen("sample.wav","rb");assert(f);
    assert(fread(data,1,sizeof(data),f)==size);fclose(f);
    assert(!memcmp(data,expected,size));
    assert(access("sample.wav.part",F_OK)!=0);
}
static HAL_StatusTypeDef run(void) {
    request_t request={0};strcpy(request.path,"sample.wav");strcpy(request.json,"{\"op\":\"test\"}");
    return download(&request);
}
static void reset(void) {
    head=tail=0;overflow=0;parser.used=0;ticks=0;
    last_end_time=0;last_end_id=last_end_seq=0;
    duplicate_data=write_failure=install_failure=bad_crc=no_response=ack_count=drop_requests=request_count=0;
    pause_after_begin=0;pause_requested=paused=0;
    source_size=sizeof(payload);storage_ready=1;
    original_file();
}
int main(void) {
    Transfer_Init();assert(!debug_enabled);
    char folder[]="/tmp/acho-transfer-test-XXXXXX";
    assert(mkdtemp(folder));assert(chdir(folder)==0);
    for(size_t i=0;i<sizeof(payload);i++) payload[i]=(uint8_t)i;
    reset();assert(run()==HAL_OK);check_file(payload,sizeof(payload));
    TransferStatus diag;Transfer_GetStatus(&diag);
    assert(diag.stage==TRANSFER_DONE && diag.result==HAL_OK && diag.rx_frames && diag.rx_bytes);
    assert(diag.received==sizeof(payload) && diag.total==sizeof(payload) && diag.last_id==diag.request_id);
    reset();duplicate_data=1;assert(run()==HAL_OK);check_file(payload,sizeof(payload));assert(ack_count==5);
    /* Repeated END is ACKed after installation, without installing twice. */
    uint8_t end[8];acho_put32(end,source_size);acho_put32(end+4,acho_crc32(0,payload,source_size));
    inject(ACHO_END,peer_seq,end,8);assert(receive_frame(1)==1);assert(old_end());check_file(payload,sizeof(payload));
    reset();source_size=0;assert(run()==HAL_OK);check_file(payload,0);
    reset();bad_crc=1;assert(run()==HAL_ERROR);check_file("original",8);
    reset();write_failure=1;assert(run()==HAL_ERROR);check_file("original",8);
    reset();install_failure=1;assert(run()==HAL_ERROR);check_file("original",8);
    reset();no_response=1;assert(run()==HAL_TIMEOUT);check_file("original",8);
    Transfer_GetStatus(&diag);
    assert(diag.stage==TRANSFER_WAIT_BEGIN && diag.result==HAL_TIMEOUT && !diag.rx_bytes && !diag.rx_frames);
    reset();link_status=(TransferStatus){.stage=TRANSFER_WAIT_BEGIN};
    irq_byte=0xff;HAL_UART_RxCpltCallback(&huart2);assert(receive_frame(1)==0);
    Transfer_GetStatus(&diag);assert(diag.rx_bytes==1 && diag.rx_frames==0);
    HAL_UART_ErrorCallback(&huart2);assert(receive_frame(1)==-1);
    Transfer_GetStatus(&diag);assert(diag.uart_errors==1);
    reset();storage_ready=0;assert(run()==HAL_ERROR);check_file("original",8);
    /* Provisioning info/QR must work without FatFs or an SD card. */
    uint8_t memory[1801]; size_t received_size=99;
    request_t control={.memory=memory,.capacity=1800,.received=&received_size};
    strcpy(control.json,"{\"op\":\"get_wifi_qr\"}");
    reset();storage_ready=0;memset(memory,0x5a,sizeof(memory));
    assert(download(&control)==HAL_OK); assert(received_size==1800);
    assert(!memcmp(memory,payload,1800)); assert(memory[1800]==0x5a); check_file("original",8);
    reset();storage_ready=0;drop_requests=2;
    assert(download(&control)==HAL_OK && received_size==1800 && drop_requests==0);
    reset();storage_ready=0;control.capacity=32;received_size=99;memset(memory,0x5a,sizeof(memory));
    assert(download(&control)==HAL_ERROR); assert(received_size==0);
    for(size_t i=0;i<sizeof(memory);i++) assert(memory[i]==0x5a);
    reset();storage_ready=0;control.capacity=1800;bad_crc=1;received_size=99;
    assert(download(&control)==HAL_ERROR); assert(received_size==0); check_file("original",8);
    /* Network pairing token may need registration + WS setup: no local 1s retries. */
    reset();control.initial_timeout_ms=120000;no_response=1;
    assert(download(&control)==HAL_TIMEOUT && ticks>120000 && request_count==1);
    reset();control.initial_timeout_ms=10000;no_response=1;
    assert(download(&control)==HAL_TIMEOUT && ticks>10000 && ticks<11000 && request_count==3);
    /* Interrupt an open .part transfer without transmitting into the ROM loader. */
    reset();pause_after_begin=1;
    assert(run()==HAL_BUSY);check_file("original",8);assert(ack_count==1);
    assert(!Transfer_IsPaused()); /* Requesting pause is not a hardware acknowledgement. */
    set_paused(1);assert(Transfer_IsPaused() && !uart_enabled && !irq_enabled && gpio_mode==GPIO_MODE_INPUT);
    unsigned starts=receive_starts;
    HAL_UART_RxCpltCallback(&huart2);HAL_UART_ErrorCallback(&huart2);
    assert(head==0 && receive_starts==starts);
    received_size=99;assert(download(&control)==HAL_BUSY && received_size==0);
    assert(!Transfer_SubmitMemory("{}",memory,sizeof(memory),&received_size,NULL,NULL));
    assert(!Transfer_Submit("{}","sample.wav",NULL,NULL));
    Transfer_SetPaused(0);assert(Transfer_IsPaused());
    assert(!Transfer_SubmitMemory("{}",memory,sizeof(memory),&received_size,NULL,NULL));
    set_paused(0);assert(!Transfer_IsPaused() && uart_enabled && irq_enabled && gpio_mode==GPIO_MODE_AF_PP);
    assert(!debug_enabled);
    assert(receive_starts==starts+1 && head==tail && parser.used==0);
    reset();assert(run()==HAL_OK);check_file(payload,sizeof(payload));
    unlink("sample.wav");chdir("/");rmdir(folder);
    puts("STM32 transfer: chunks, duplicate DATA/END, CRC/write/rename failures, RAM controls, pause cleanup, high-Z pins, IRQ suppression and resume passed");
}
