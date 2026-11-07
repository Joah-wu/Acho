#ifndef ACHO_PROTOCOL_H
#define ACHO_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define ACHO_CHUNK 1024
#define ACHO_HEADER 16
#define ACHO_MAX_FILE (32u * 1024u * 1024u)
enum { ACHO_REQUEST=1, ACHO_BEGIN, ACHO_DATA, ACHO_END, ACHO_ERROR, ACHO_ACK };
typedef struct {
    uint8_t type;
    uint16_t id, seq, len;
    uint8_t data[ACHO_CHUNK];
} acho_frame_t;
typedef struct {
    uint8_t bytes[ACHO_HEADER + ACHO_CHUNK];
    size_t used;
} acho_parser_t;
static inline uint16_t acho_u16(const uint8_t *p) { return p[0] | ((uint16_t)p[1]<<8); }
static inline uint32_t acho_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static inline void acho_put16(uint8_t *p, uint16_t n) { p[0]=n; p[1]=n>>8; }
static inline void acho_put32(uint8_t *p, uint32_t n) { for(int i=0;i<4;i++) p[i]=n>>(8*i); }
/* Same incremental IEEE CRC32 convention as Python zlib.crc32. */
static inline uint32_t acho_crc32(uint32_t crc, const void *data, size_t len) {
    const uint8_t *p=data;
    crc=~crc;
    while(len--) {
        crc^=*p++;
        for(int i=0;i<8;i++) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1)));
    }
    return ~crc;
}
static inline size_t acho_encode(uint8_t *out, const acho_frame_t *f) {
    if(f->len>ACHO_CHUNK) return 0;
    memset(out,0,ACHO_HEADER);
    out[0]=0xa5; out[1]=0x5a; out[2]=1; out[3]=f->type;
    acho_put16(out+4,f->id); acho_put16(out+6,f->seq); acho_put16(out+8,f->len);
    memcpy(out+ACHO_HEADER,f->data,f->len);
    acho_put32(out+12,acho_crc32(acho_crc32(0,out,12),f->data,f->len));
    return ACHO_HEADER+f->len;
}
/* Byte stream parser resynchronizes after noise and corrupt headers/payloads. */
static inline int acho_feed(acho_parser_t *p, uint8_t byte, acho_frame_t *f) {
    p->bytes[p->used++]=byte;
    for(;;) {
        if(p->used==0) return 0;
        if(p->bytes[0]!=0xa5 || (p->used>=2 && p->bytes[1]!=0x5a)) goto shift;
        if(p->used<ACHO_HEADER) return 0;
        uint8_t *b=p->bytes;
        size_t len=acho_u16(b+8);
        if(b[2]!=1 || b[3]<ACHO_REQUEST || b[3]>ACHO_ACK || len>ACHO_CHUNK || b[10] || b[11]) goto shift;
        if(p->used<ACHO_HEADER+len) return 0;
        if(acho_u32(b+12)!=acho_crc32(acho_crc32(0,b,12),b+ACHO_HEADER,len)) goto shift;
        f->type=b[3]; f->id=acho_u16(b+4); f->seq=acho_u16(b+6); f->len=len;
        memcpy(f->data,b+ACHO_HEADER,len);
        p->used-=ACHO_HEADER+len;
        memmove(b,b+ACHO_HEADER+len,p->used);
        return 1;
shift:
        memmove(p->bytes,p->bytes+1,--p->used);
    }
}
#endif
