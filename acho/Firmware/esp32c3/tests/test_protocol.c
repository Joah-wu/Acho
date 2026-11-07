#include "acho_protocol.h"
#include <assert.h>
#include <stdio.h>
static acho_parser_t p;
static acho_frame_t f, received;
static uint8_t data[ACHO_HEADER+ACHO_CHUNK];
static int feed(const uint8_t *bytes, size_t len) {
    int count=0;
    for(size_t i=0;i<len;i++) count+=acho_feed(&p,bytes[i],&received);
    return count;
}
int main(void) {
    assert(acho_crc32(0,"123456789",9)==0xcbf43926u);
    assert(acho_crc32(acho_crc32(0,"1234",4),"56789",5)==0xcbf43926u);
    for(size_t len=0;len<=ACHO_CHUNK;len++) {
        f.type=ACHO_DATA; f.id=0x1234; f.seq=len; f.len=len;
        for(size_t i=0;i<len;i++) f.data[i]=(uint8_t)(i*19);
        size_t n=acho_encode(data,&f);
        assert(n==ACHO_HEADER+len);
        assert(feed(data,n)==1);
        assert(received.id==f.id && received.seq==f.seq && received.type==f.type);
        assert(received.len==len && !memcmp(received.data,f.data,len));
    }
    uint8_t noise[]={0x10,0xa5,0xa5,0x00,0x5a};
    assert(feed(noise,sizeof(noise))==0);
    f.len=9; memcpy(f.data,"123456789",9);
    size_t n=acho_encode(data,&f);
    assert(feed(data,n)==1);
    data[ACHO_HEADER+3]^=0x80;
    assert(feed(data,n)==0);
    acho_encode(data,&f);
    assert(feed(data,n)==1); /* resynchronize after corrupt payload */
    data[8]=0xff; data[9]=0xff;
    assert(feed(data,n)==0);
    acho_encode(data,&f);
    assert(feed(data,n)==1);
    /* Partial frame followed by garbage and a valid frame. */
    assert(feed(data,7)==0);
    uint8_t zeros[1040]={0};
    feed(zeros,sizeof(zeros));
    assert(feed(data,n)==1);
    puts("protocol: CRC vectors, 1025 frame sizes, noise, corruption, resynchronization passed");
}
