#include "nfc_tag.h"
#include "st25dv.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_usart.h"
#include <stdio.h>
#include <string.h>

static ST25DV_Object_t tag;
static uint16_t data_address=ST25DV_ADDR_DATA_I2C;
static struct {
    uint16_t address,reg;
    HAL_StatusTypeDef status;
    uint32_t error,state;
    uint8_t reference;
    char operation;
} io_trace;
static uint32_t last_diagnostic_at;
static int diagnostic_started;
static int diagnostic_due(void) {
    uint32_t now=HAL_GetTick();
    if(diagnostic_started && (uint32_t)(now-last_diagnostic_at)<5000U) return 0;
    diagnostic_started=1;last_diagnostic_at=now;return 1;
}
static void trace_io(uint16_t slave,uint16_t reg,HAL_StatusTypeDef result,char operation) {
    io_trace.address=slave;io_trace.reg=reg;io_trace.status=result;
    io_trace.operation=operation;
    io_trace.error=HAL_I2C_GetError(&hi2c2);
    io_trace.state=(uint32_t)HAL_I2C_GetState(&hi2c2);
}
static void report_failure(const char *stage,int enabled) {
    if(!enabled) return;
    /* Keep below Printf's 128-byte buffer. Never print EEPROM credentials. */
    Printf("NFC %s: a7=%02X reg=%04X op=%c st=%u err=%08lX hs=%02lX id=%02X SCL=%u SDA=%u\r\n",
        stage,(unsigned)(io_trace.address>>1),(unsigned)io_trace.reg,io_trace.operation,(unsigned)io_trace.status,
        (unsigned long)io_trace.error,(unsigned long)io_trace.state,(unsigned)io_trace.reference,
        (unsigned)HAL_GPIO_ReadPin(ST25D_I2C_SCL_GPIO_Port,ST25D_I2C_SCL_Pin),
        (unsigned)HAL_GPIO_ReadPin(ST25D_I2C_SDA_GPIO_Port,ST25D_I2C_SDA_Pin));
}
static uint16_t address(uint16_t driver_address) {
    return driver_address==ST25DV_ADDR_SYST_I2C?data_address+8:data_address;
}
static int32_t io_init(void) { return NFCTAG_OK; } /* MX_I2C2_Init is called by main. */
static int32_t io_ready(uint16_t slave,uint32_t trials) {
    uint16_t actual=address(slave);
    HAL_StatusTypeDef result=HAL_I2C_IsDeviceReady(&hi2c2,actual,trials,5);
    trace_io(actual,0,result,'P');
    if(result!=HAL_OK) vTaskDelay(pdMS_TO_TICKS(1));
    return result==HAL_OK?NFCTAG_OK:NFCTAG_ERROR;
}
static int32_t io_read(uint16_t slave,uint16_t reg,uint8_t *data,uint16_t length) {
    uint16_t actual=address(slave);
    HAL_StatusTypeDef result=HAL_I2C_Mem_Read(&hi2c2,actual,reg,I2C_MEMADD_SIZE_16BIT,data,length,50);
    trace_io(actual,reg,result,'R');
    return result==HAL_OK?NFCTAG_OK:NFCTAG_ERROR;
}
static int32_t io_write(uint16_t slave,uint16_t reg,const uint8_t *data,uint16_t length) {
    uint16_t actual=address(slave);
    HAL_StatusTypeDef result=HAL_I2C_Mem_Write(&hi2c2,actual,reg,I2C_MEMADD_SIZE_16BIT,(uint8_t*)data,length,50);
    trace_io(actual,reg,result,'W');
    return result==HAL_OK?NFCTAG_OK:NFCTAG_ERROR;
}
static int identify_tag(void) {
    uint8_t reference;
    if(St25Dv_Drv.ReadID(&tag,&reference)!=NFCTAG_OK) return -1;
    io_trace.reference=reference;
    /* RegisterBusIO already initialized the bus. This component's generated
     * ST25DV_Init accepts only the older 04/64 IDs. Accept legacy CMOS too,
     * plus the KC IDs 0x50/0x51 (open-drain), 0x52/0x53 (CMOS): ST AN5675.
     * Keep this board compatibility check outside generated component files. */
    if(!ST25DV_AM_I_OPEN_DRAIN(reference) && !ST25DV_AM_I_CMOS(reference) &&
       (reference<0x50U || reference>0x53U)) return -1;
    tag.IsInitialized=1U;
    return 0;
}
int NfcTag_Init(NfcTagInfo *info) {
    int log_this_probe=diagnostic_due();
    io_trace.reference=0;
    memset(&tag,0,sizeof(tag));memset(info,0,sizeof(*info));
    ST25DV_IO_t io={.Init=io_init,.IsReady=io_ready,.Read=io_read,.Write=io_write,.GetTick=HAL_GetTick};
    if(ST25DV_RegisterBusIO(&tag,&io)!=NFCTAG_OK) { report_failure("register",log_this_probe);return -1; }
    /* E2 address selection on ST25DV boards may select either 0x53 or 0x51. */
    for(unsigned attempt=0;attempt<2;attempt++) {
        data_address=attempt?0xa2:0xa6;
        if(St25Dv_Drv.IsReady(&tag,1)!=NFCTAG_OK) {
            report_failure("probe-data",log_this_probe);continue;
        }
        if(io_ready(ST25DV_ADDR_SYST_I2C,1)!=NFCTAG_OK) {
            report_failure("probe-system",log_this_probe);continue;
        }
        if(identify_tag()!=0) {
            report_failure(io_trace.status==HAL_OK?"unsupported-id":"read-id",log_this_probe);continue;
        }
        {
            ST25DV_MEM_SIZE size;ST25DV_UID uid;
            if(ST25DV_ReadMemSize(&tag,&size)!=NFCTAG_OK) { report_failure("memory-size",log_this_probe);return -1; }
            if(ST25DV_ReadUID(&tag,&uid)!=NFCTAG_OK) { report_failure("uid",log_this_probe);return -1; }
            info->ic_reference=io_trace.reference;
            info->memory_bytes=((uint32_t)size.Mem_Size+1u)*((uint32_t)size.BlockSize+1u);
            if(info->memory_bytes<ACHO_NFC_MEMORY) { report_failure("small-memory",log_this_probe);return -1; }
            snprintf(info->uid,sizeof(info->uid),"%08lX%08lX",(unsigned long)uid.MsbUid,(unsigned long)uid.LsbUid);
            if(log_this_probe) Printf("NFC identity: a7=%02X id=%02X bytes=%lu uid=%s\r\n",
                (unsigned)(data_address>>1),(unsigned)info->ic_reference,(unsigned long)info->memory_bytes,info->uid);
            /* MB_MODE=0 forces MB_EN=0 (DS10925). Do not require a mailbox
             * dynamic-register transaction when static FTM mode is off. */
            ST25DV_EN_STATUS mode,enabled;
            if(ST25DV_ReadMBMode(&tag,&mode)!=NFCTAG_OK) { report_failure("mailbox-mode",log_this_probe);return -1; }
            if(mode==ST25DV_ENABLE) {
                if(ST25DV_GetMBEN_Dyn(&tag,&enabled)!=NFCTAG_OK) { report_failure("mailbox-status",log_this_probe);return -1; }
                if(enabled==ST25DV_ENABLE) {
                    if(ST25DV_ResetMBEN_Dyn(&tag)!=NFCTAG_OK) { report_failure("mailbox-off",log_this_probe);return -1; }
                    if(ST25DV_GetMBEN_Dyn(&tag,&enabled)!=NFCTAG_OK || enabled!=ST25DV_DISABLE) {
                        report_failure("mailbox-verify",log_this_probe);return -1;
                    }
                }
            }
            if(log_this_probe) Printf("NFC ready: a7=%02X mb_mode=%u bytes=%lu\r\n",
                (unsigned)(data_address>>1),(unsigned)mode,(unsigned long)info->memory_bytes);
            return 0;
        }
    }
    return -1;
}
int NfcTag_FieldPresent(void) {
    ST25DV_FIELD_STATUS field;
    if(ST25DV_GetRFField_Dyn(&tag,&field)!=NFCTAG_OK) { report_failure("rf-field",diagnostic_due());return -1; }
    return field==ST25DV_FIELD_ON;
}
static int read_memory(void *context,uint16_t offset,uint8_t *data,uint16_t size) {
    (void)context;return St25Dv_Drv.ReadData(&tag,data,offset,size)==NFCTAG_OK?0:-1;
}
static int write_memory(void *context,uint16_t offset,const uint8_t *data,uint16_t size) {
    (void)context;return St25Dv_Drv.WriteData(&tag,data,offset,size)==NFCTAG_OK?0:-1;
}
int NfcTag_Publish(const uint8_t image[ACHO_NFC_MEMORY]) {
    acho_ndef_io_t io={.read=read_memory,.write=write_memory};
    int result=acho_ndef_publish(&io,image);
    if(result) report_failure("publish",diagnostic_due());
    return result;
}
