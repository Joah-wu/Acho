package com.example.websocktest.core;

import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/** Bounded NFC Forum Type 5 / NDEF decoder shared by native and raw NFC-V reads. */
public final class NfcPayload {
    public static final String MIME="application/vnd.acho.device";
    public static final int MEMORY_SIZE=512, MAX_PAYLOAD=384;
    private NfcPayload() {}
    private static IllegalArgumentException invalid() { return new IllegalArgumentException("未读到有效的 Acho NFC 数据，请在设备菜单选择配网或账号绑定后重试"); }
    private static int unsigned(byte value) { return value & 255; }
    public static String decodeTag(byte[] memory) {
        if(memory==null || memory.length<8 || memory.length>MEMORY_SIZE) throw invalid();
        int magic=unsigned(memory[0]), cursor=magic==0xe1?4:magic==0xe2?8:0;
        if(cursor==0 || (unsigned(memory[1]) & 0xc0)!=0x40 || (memory[1]&0x0c)!=0) throw invalid();
        while(cursor<memory.length) {
            int type=unsigned(memory[cursor++]);
            if(type==0) continue;
            if(type==0xfe) break;
            if(cursor>=memory.length) throw invalid();
            int size=unsigned(memory[cursor++]);
            if(size==255) {
                if(cursor+2>memory.length) throw invalid();
                size=(unsigned(memory[cursor])<<8)|unsigned(memory[cursor+1]); cursor+=2;
            }
            if(size>memory.length-cursor) throw invalid();
            if(type==3) return decodeMessage(Arrays.copyOfRange(memory,cursor,cursor+size));
            cursor+=size;
        }
        throw invalid();
    }
    public static String decodeMessage(byte[] message) {
        if(message==null || message.length==0 || message.length>MEMORY_SIZE) throw invalid();
        int cursor=0, records=0; String result=null; boolean ended=false;
        while(cursor<message.length) {
            if(ended || cursor+3>message.length) throw invalid();
            int flags=unsigned(message[cursor++]), typeLength=unsigned(message[cursor++]);
            if(((flags&0x80)!=0)!=(records==0) || (flags&0x20)!=0) throw invalid();
            long length=0;
            int lengthBytes=(flags&0x10)!=0?1:4;
            if(cursor+lengthBytes>message.length) throw invalid();
            for(int i=0;i<lengthBytes;i++) length=(length<<8)|unsigned(message[cursor++]);
            int idLength=0;
            if((flags&8)!=0) { if(cursor>=message.length) throw invalid(); idLength=unsigned(message[cursor++]); }
            if(length>message.length-cursor-typeLength-idLength || length<0) throw invalid();
            String type=new String(message,cursor,typeLength,StandardCharsets.US_ASCII);
            cursor+=typeLength+idLength;
            if((flags&7)==2 && MIME.equals(type)) {
                if(result!=null || length==0 || length>MAX_PAYLOAD) throw invalid();
                try {
                    result=StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
                            .onUnmappableCharacter(CodingErrorAction.REPORT)
                            .decode(ByteBuffer.wrap(message,cursor,(int)length)).toString();
                } catch(CharacterCodingException error) { throw invalid(); }
            }
            cursor+=(int)length; records++; ended=(flags&0x40)!=0;
        }
        if(!ended || result==null) throw invalid();
        return result;
    }
}
