package com.example.websocktest.core;

import org.junit.Test;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Random;
import static org.junit.Assert.*;

public class NfcPayloadTest {
    private byte[] fixture(String name) throws Exception {
        try(InputStream stream=getClass().getResourceAsStream("/nfc/acho-nfc-"+name+".bin")) {
            assertNotNull(stream); return stream.readAllBytes();
        }
    }
    private void rejected(byte[] tag) {
        try { NfcPayload.decodeTag(tag); fail("Invalid tag accepted"); }
        catch(IllegalArgumentException expected) { }
    }
    @Test public void stm32ProvisioningFixtures() throws Exception {
        for(String name:new String[]{"wifi","wifi-e2"}) {
            QrPayload payload=QrPayload.parse(NfcPayload.decodeTag(fixture(name)));
            assertTrue(payload.provisioning); assertEquals("ACHO_AABBCC",payload.name);
            assertEquals("0123456789abcdef",payload.pop); assertEquals(payload.pop,payload.password);
        }
    }
    @Test public void extendedTlvAndLongRecord() throws Exception {
        assertEquals("a".repeat(384),NfcPayload.decodeTag(fixture("long")));
        assertEquals("{\"purpose\":\"idle\"}",NfcPayload.decodeTag(fixture("idle")));
    }
    @Test public void invalidBoundsMimeUtf8AndIncompletePublication() throws Exception {
        byte[] valid=fixture("wifi");
        for(int n=0;n<6+(valid[5]&255);n++) rejected(Arrays.copyOf(valid,n));
        byte[] changed=valid.clone(); changed[5]=0; rejected(changed);
        changed=valid.clone(); changed[0]=0; rejected(changed);
        changed=valid.clone(); changed[1]=0x4f; rejected(changed);
        changed=valid.clone(); changed[5]=(byte)255; changed[6]=(byte)255; changed[7]=(byte)255; rejected(changed);
        changed=valid.clone(); changed[9]='x'; rejected(changed); // MIME mismatch
        changed=valid.clone(); changed[6]|=0x20; rejected(changed); // Unsupported chunked record
        changed=valid.clone(); changed[6]&=0x7f; rejected(changed); // Missing message begin
        changed=valid.clone(); changed[9+NfcPayload.MIME.length()]=(byte)0xff; rejected(changed);
        changed=valid.clone(); changed[5]--; rejected(changed); // Truncated application record
        rejected(Arrays.copyOf(valid,513));
    }
    @Test public void randomInputsFailWithinBounds() {
        Random random=new Random(2504);
        for(int i=0;i<10000;i++) {
            byte[] bytes=new byte[random.nextInt(514)];random.nextBytes(bytes);
            rejected(bytes);
        }
    }
}
