#include "acho_ndef.h"
#include <string.h>

static size_t record(uint8_t *out,uint8_t flags,const char *type,const uint8_t *payload,size_t size) {
    size_t type_size=strlen(type), cursor=0;
    int short_record=size<256;
    out[cursor++]=flags|(short_record?0x10:0);
    out[cursor++]=(uint8_t)type_size;
    if(short_record) out[cursor++]=(uint8_t)size;
    else for(int i=3;i>=0;i--) out[cursor++]=(uint8_t)(size>>(i*8));
    memcpy(out+cursor,type,type_size);cursor+=type_size;
    memcpy(out+cursor,payload,size);return cursor+size;
}
size_t acho_ndef_encode(uint8_t *out,size_t capacity,const uint8_t *payload,size_t length,int extended_cc) {
    if(!out || !payload || !length || length>ACHO_NFC_MAX_PAYLOAD || capacity<ACHO_NFC_MEMORY) return 0;
    /* MIME first, Android application record last. Neither is a web URI. */
    size_t message_size=(length<256?3u:6u)+strlen(ACHO_NFC_MIME)+length+
        3u+strlen("android.com:pkg")+strlen(ACHO_ANDROID_PACKAGE);
    size_t cc=extended_cc?8u:4u, tlv=message_size<255?2u:4u;
    size_t used=cc+tlv+message_size+1;
    if(used>ACHO_NFC_MEMORY) return 0;
    memset(out,0,ACHO_NFC_MEMORY);
    out[0]=extended_cc?0xe2:0xe1;out[1]=0x43; /* Mapping 1.0, RF NDEF read-only. */
    if(extended_cc) { out[6]=0;out[7]=63; } /* 504-byte data area after 8-byte CC. */
    else out[2]=64; /* ST's smartphone-compatible ST25DV04 CC memory encoding. */
    size_t p=cc;out[p++]=0x03;
    if(tlv==2) out[p++]=(uint8_t)message_size;
    else { out[p++]=0xff;out[p++]=message_size>>8;out[p++]=message_size; }
    p+=record(out+p,0x82,ACHO_NFC_MIME,payload,length);
    p+=record(out+p,0x44,"android.com:pkg",(const uint8_t*)ACHO_ANDROID_PACKAGE,strlen(ACHO_ANDROID_PACKAGE));
    out[p++]=0xfe;
    return p;
}
int acho_ndef_publish(const acho_ndef_io_t *io,const uint8_t image[ACHO_NFC_MEMORY]) {
    if(!io || !io->read || !io->write || !image || (image[0]!=0xe1 && image[0]!=0xe2)) return -1;
    uint16_t cc=image[0]==0xe1?4:8;
    uint8_t invalid[4]={0x03,0,0xfe,0}, current[16];
    if(io->write(io->context,cc,invalid,sizeof(invalid))) return -1;
    /* Compare before writing to limit EEPROM wear; also erase old credential tails. */
    for(uint16_t offset=0;offset<ACHO_NFC_MEMORY;offset+=sizeof(current)) {
        if(io->read(io->context,offset,current,sizeof(current))) return -1;
        uint8_t next[16];memcpy(next,image+offset,sizeof(next));
        if(cc>=offset && cc<offset+sizeof(next)) memcpy(next+cc-offset,invalid,sizeof(invalid));
        if(memcmp(next,current,sizeof(next)) && io->write(io->context,offset,next,sizeof(next))) return -1;
        if(io->read(io->context,offset,current,sizeof(current)) || memcmp(next,current,sizeof(next))) return -1;
    }
    /* A single aligned 4-byte EEPROM page publishes the TLV length last. */
    if(io->write(io->context,cc,image+cc,4)) return -1;
    if(io->read(io->context,cc,current,4) || memcmp(current,image+cc,4)) {
        io->write(io->context,cc,invalid,sizeof(invalid));return -1;
    }
    return 0;
}
