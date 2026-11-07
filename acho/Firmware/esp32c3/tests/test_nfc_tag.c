/* Actual board adapter + generated ST component, with an I2C register model. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "i2c.h"
#include "FreeRTOS.h"
#include "nfc_tag.h"
#include "st25dv.h"

I2C_HandleTypeDef hi2c2;
static uint8_t system_registers[256],dynamic_registers[8],memory[ACHO_NFC_MEMORY];
static uint16_t data_address;
static uint32_t tick;
static int fail_id,fail_mailbox,fail_memory;
static int fail_mailbox_read,fail_mailbox_mode,ignore_mailbox_clear;
static unsigned mailbox_reads,mailbox_writes;
static int fail_system_probe;
static HAL_StatusTypeDef probe_result=HAL_OK;
static unsigned log_count;
static char diagnostic_log[4096];

int Printf(char *format,...) {
    va_list args;
    char line[128];
    va_start(args,format);int length=vsnprintf(line,sizeof(line),format,args);va_end(args);
    assert(length>=0 && (size_t)length<sizeof(line));
    assert(strlen(diagnostic_log)+(size_t)length<sizeof(diagnostic_log));
    strcat(diagnostic_log,line);log_count++;return length;
}
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef *bus) { assert(bus==&hi2c2);return 0; }
uint32_t HAL_I2C_GetState(I2C_HandleTypeDef *bus) { assert(bus==&hi2c2);return 0x20; }
GPIO_PinState HAL_GPIO_ReadPin(void *port,uint16_t pin) {
    assert(port==ST25D_I2C_SCL_GPIO_Port);
    assert(pin==ST25D_I2C_SCL_Pin || pin==ST25D_I2C_SDA_Pin);return GPIO_PIN_SET;
}

uint32_t HAL_GetTick(void) { return tick++; }
void vTaskDelay(TickType_t ticks) { tick+=ticks; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *bus,uint16_t slave,uint32_t trials,uint32_t timeout) {
    (void)trials;(void)timeout;assert(bus==&hi2c2);
    if(probe_result!=HAL_OK) return probe_result;
    if(fail_system_probe && slave==data_address+8) return HAL_ERROR;
    return slave==data_address || slave==data_address+8?HAL_OK:HAL_ERROR;
}
static uint8_t *region(uint16_t slave,uint16_t reg,uint16_t length) {
    if(slave==data_address+8 && reg<sizeof(system_registers) && length<=sizeof(system_registers)-reg)
        return system_registers+reg;
    if(slave!=data_address) return NULL;
    if(reg>=0x2000U && reg-0x2000U<sizeof(dynamic_registers) && length<=sizeof(dynamic_registers)-(reg-0x2000U))
        return dynamic_registers+reg-0x2000U;
    if(reg<sizeof(memory) && length<=sizeof(memory)-reg) return memory+reg;
    return NULL;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *bus,uint16_t slave,uint16_t reg,uint16_t address_size,uint8_t *out,uint16_t length,uint32_t timeout) {
    (void)timeout;assert(bus==&hi2c2 && address_size==I2C_MEMADD_SIZE_16BIT);
    if(fail_id && reg==ST25DV_ICREF_REG) return HAL_ERROR;
    if(fail_mailbox_mode && reg==ST25DV_MB_MODE_REG) return HAL_ERROR;
    if(reg==ST25DV_MB_CTRL_DYN_REG) {
        mailbox_reads++;
        if(fail_mailbox_read) return HAL_ERROR;
    }
    uint8_t *source=region(slave,reg,length);
    if(!source) return HAL_ERROR;
    memcpy(out,source,length);return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *bus,uint16_t slave,uint16_t reg,uint16_t address_size,uint8_t *in,uint16_t length,uint32_t timeout) {
    (void)timeout;assert(bus==&hi2c2 && address_size==I2C_MEMADD_SIZE_16BIT);
    if((fail_mailbox && reg==ST25DV_MB_CTRL_DYN_REG) || (fail_memory && reg<sizeof(memory))) return HAL_ERROR;
    if(reg==ST25DV_MB_CTRL_DYN_REG) {
        mailbox_writes++;
        if(ignore_mailbox_clear) return HAL_OK;
    }
    uint8_t *target=region(slave,reg,length);
    if(!target) return HAL_ERROR;
    memcpy(target,in,length);return HAL_OK;
}
static void fixture(uint8_t reference,uint16_t slave) {
    memset(system_registers,0,sizeof(system_registers));
    memset(dynamic_registers,0,sizeof(dynamic_registers));
    memset(memory,0x55,sizeof(memory));
    data_address=slave;fail_id=fail_mailbox=fail_memory=fail_system_probe=0;
    probe_result=HAL_OK;tick+=6000;
    fail_mailbox_read=fail_mailbox_mode=ignore_mailbox_clear=0;
    mailbox_reads=mailbox_writes=0;
    system_registers[ST25DV_ICREF_REG]=reference;
    int small=reference==0x24 || reference==0x25 || reference==0x50 || reference==0x52;
    system_registers[ST25DV_MEM_SIZE_LSB_REG]=small?0x7f:0xff;
    system_registers[ST25DV_MEM_SIZE_MSB_REG]=small?0:7;
    system_registers[ST25DV_BLK_SIZE_REG]=3;
    system_registers[ST25DV_MB_MODE_REG]=1;
    for(unsigned i=0;i<8;i++) system_registers[ST25DV_UID_REG+i]=(uint8_t)(i+1);
    dynamic_registers[ST25DV_MB_CTRL_DYN_REG-0x2000]=1;
}
int main(void) {
    static const uint8_t references[]={0x24,0x25,0x26,0x27,0x50,0x51,0x52,0x53};
    NfcTagInfo info;
    fixture(0x51,0xa6);probe_result=HAL_ERROR;
    assert(NfcTag_Init(&info)==-1);
    assert(strstr(diagnostic_log,"probe-data: a7=53") && strstr(diagnostic_log,"probe-data: a7=51"));
    assert(strstr(diagnostic_log,"st=1 err=00000000 hs=20") && strstr(diagnostic_log,"SCL=1 SDA=1"));
    unsigned logs=log_count;assert(NfcTag_Init(&info)==-1 && log_count==logs);
    fixture(0x51,0xa6);probe_result=HAL_BUSY;
    assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"st=2"));
    fixture(0x51,0xa6);fail_system_probe=1;
    assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"probe-system: a7=57"));
    diagnostic_log[0]=0;
    for(size_t i=0;i<sizeof(references);i++) {
        for(unsigned address=0;address<2;address++) {
            fixture(references[i],address?0xa2:0xa6);
            assert(NfcTag_Init(&info)==0 && info.ic_reference==references[i]);
            unsigned expected=(references[i]==0x24 || references[i]==0x25 || references[i]==0x50 || references[i]==0x52)?512:8192;
            assert(info.memory_bytes==expected && strlen(info.uid)==16);
            assert(!(dynamic_registers[ST25DV_MB_CTRL_DYN_REG-0x2000]&1));
            assert(mailbox_reads==3 && mailbox_writes==1);
        }
    }
    diagnostic_log[0]=0;
    fixture(0x51,0xa6);fail_id=1;assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"read-id"));
    fixture(0xff,0xa6);assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"unsupported-id"));
    fixture(0x51,0xa6);fail_mailbox=1;assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"mailbox-off"));
    fixture(0x24,0xa6);fail_mailbox_mode=1;
    assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"mailbox-mode"));
    fixture(0x24,0xa6);fail_mailbox_read=1;
    assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"mailbox-status"));
    fixture(0x24,0xa6);ignore_mailbox_clear=1;
    assert(NfcTag_Init(&info)==-1 && strstr(diagnostic_log,"mailbox-verify"));
    fixture(0x24,0xa6);dynamic_registers[ST25DV_MB_CTRL_DYN_REG-0x2000]=0;
    assert(NfcTag_Init(&info)==0 && mailbox_reads==1 && mailbox_writes==0);
    /* Match the reported 04K: reject 0x2006, but keep static FTM off. */
    fixture(0x24,0xa6);system_registers[ST25DV_MB_MODE_REG]=0;
    dynamic_registers[ST25DV_MB_CTRL_DYN_REG-0x2000]=0;
    fail_mailbox_read=fail_mailbox=1;
    assert(NfcTag_Init(&info)==0 && info.memory_bytes==512);
    assert(mailbox_reads==0 && mailbox_writes==0);
    assert(NfcTag_FieldPresent()==0);
    uint8_t small_image[ACHO_NFC_MEMORY];
    assert(acho_ndef_encode(small_image,sizeof(small_image),(const uint8_t *)ACHO_NFC_IDLE,strlen(ACHO_NFC_IDLE),0));
    assert(small_image[0]==0xe1 && NfcTag_Publish(small_image)==0);
    assert(!memcmp(memory,small_image,sizeof(memory)));
    diagnostic_log[0]=0;
    fixture(0x51,0xa6);system_registers[ST25DV_MEM_SIZE_LSB_REG]=0;system_registers[ST25DV_MEM_SIZE_MSB_REG]=0;
    assert(NfcTag_Init(&info)==-1);
    fixture(0x51,0xa6);assert(NfcTag_Init(&info)==0);
    dynamic_registers[ST25DV_EH_CTRL_DYN_REG-0x2000]=0;
    assert(NfcTag_FieldPresent()==0);
    dynamic_registers[ST25DV_EH_CTRL_DYN_REG-0x2000]=ST25DV_EH_CTRL_DYN_FIELD_ON_MASK;
    assert(NfcTag_FieldPresent()==1);
    uint8_t image[ACHO_NFC_MEMORY];
    assert(acho_ndef_encode(image,sizeof(image),(const uint8_t *)ACHO_NFC_IDLE,strlen(ACHO_NFC_IDLE),1));
    assert(NfcTag_Publish(image)==0 && !memcmp(memory,image,sizeof(memory)));
    fail_memory=1;assert(NfcTag_Publish(image)==-1);
    puts("NFC tag: 04K/64K capacity, static/dynamic mailbox states, refusal and verify failure, 04K E1 NDEF, diagnostics passed");
}
