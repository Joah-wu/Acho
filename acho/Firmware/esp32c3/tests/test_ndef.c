#include "acho_ndef.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t memory[ACHO_NFC_MEMORY];
static int writes,fail_at,last_offset,last_size;
static int read_memory(void *context,uint16_t offset,uint8_t *data,uint16_t length) {
    (void)context; assert(offset+length<=sizeof(memory));memcpy(data,memory+offset,length);return 0;
}
static int write_memory(void *context,uint16_t offset,const uint8_t *data,uint16_t length) {
    (void)context; assert(offset+length<=sizeof(memory));writes++;
    if(writes==fail_at) return -1;
    last_offset=offset;last_size=length;memcpy(memory+offset,data,length);return 0;
}
static void fixture(const char *name,const uint8_t *image) {
    FILE *file=fopen(name,"wb");assert(file);assert(fwrite(image,1,ACHO_NFC_MEMORY,file)==ACHO_NFC_MEMORY);assert(!fclose(file));
}
int main(void) {
    uint8_t image[ACHO_NFC_MEMORY], payload[ACHO_NFC_MAX_PAYLOAD];memset(payload,'a',sizeof(payload));
    acho_ndef_io_t io={.read=read_memory,.write=write_memory};
    assert(!acho_ndef_encode(image,sizeof(image),payload,0,0));
    assert(!acho_ndef_encode(image,sizeof(image),payload,sizeof(payload)+1,0));
    assert(!acho_ndef_encode(image,sizeof(image)-1,payload,1,0));
    for(int extended=0;extended<2;extended++) for(size_t length=1;length<=sizeof(payload);length++) {
        size_t used=acho_ndef_encode(image,sizeof(image),payload,length,extended);
        size_t cc=extended?8:4;
        assert(used && used<=sizeof(image));assert(image[0]==(extended?0xe2:0xe1) && image[1]==0x43);
        size_t cursor=cc+2,message_length=image[cc+1];
        if(message_length==255) { message_length=((size_t)image[cc+2]<<8)|image[cc+3];cursor+=2; }
        assert(image[cursor]==(length<256?0x92:0x82));
        assert(image[cursor+1]==strlen(ACHO_NFC_MIME));
        size_t header=length<256?3:6;
        assert(!memcmp(image+cursor+header,ACHO_NFC_MIME,strlen(ACHO_NFC_MIME)));
        assert(!memcmp(image+cursor+header+strlen(ACHO_NFC_MIME),payload,length));
        assert(image[cursor+message_length]==0xfe);
        memset(memory,0x55,sizeof(memory));writes=0;fail_at=0;
        assert(!acho_ndef_publish(&io,image));assert(!memcmp(memory,image,sizeof(image)));
        assert(last_offset==(int)cc && last_size==4);
        int full_writes=writes;
        for(int failure=2;failure<=full_writes;failure++) {
            memset(memory,0x55,sizeof(memory));writes=0;fail_at=failure;
            assert(acho_ndef_publish(&io,image)==-1);
            assert(memory[cc]==3 && memory[cc+1]==0); /* Failed publication stays invalid. */
        }
    }
    const char *wifi="{\"ver\":\"v1\",\"name\":\"ACHO_AABBCC\",\"pop\":\"0123456789abcdef\",\"transport\":\"softap\",\"security\":1}";
    acho_ndef_encode(image,sizeof(image),(const uint8_t*)wifi,strlen(wifi),0);fixture("/tmp/acho-nfc-wifi.bin",image);
    acho_ndef_encode(image,sizeof(image),(const uint8_t*)wifi,strlen(wifi),1);fixture("/tmp/acho-nfc-wifi-e2.bin",image);
    acho_ndef_encode(image,sizeof(image),payload,sizeof(payload),1);fixture("/tmp/acho-nfc-long.bin",image);
    acho_ndef_encode(image,sizeof(image),(const uint8_t*)ACHO_NFC_IDLE,strlen(ACHO_NFC_IDLE),0);
    writes=0;fail_at=0;assert(!acho_ndef_publish(&io,image));assert(!memcmp(memory,image,sizeof(image)));
    fixture("/tmp/acho-nfc-idle.bin",image);
    puts("NDEF: all payload sizes, E1/E2, commit-last failure injection, credential erasure and fixtures passed");
}
