package com.example.websocktest.core;
import org.junit.Test;
import static org.junit.Assert.*;

public class ConfigurationTest {
    @Test public void normalizesServerOrigins() {
        assertEquals("http://192.168.1.50:8787",ServerAddress.normalize(" http://192.168.1.50:8787/ "));
        assertEquals("https://example.com",ServerAddress.normalize("https://example.com"));
        assertEquals("http://[::1]:8787",ServerAddress.normalize("http://[::1]:8787"));
    }
    @Test public void rejectsMalformedOrNonOriginServers() {
        for(String address:new String[]{"", "192.168.1.2", "ftp://server", "http://user:pass@server", "http://server/path", "http://server/?tok=x", "http://server/#fragment", "http://server:0", "http://server:65536"}) {
            try { ServerAddress.normalize(address); fail(address); } catch(IllegalArgumentException expected) {}
        }
    }
    @Test public void distinguishesBindingAndSecurityOneProvisioningCodes() {
        QrPayload bind=QrPayload.parse("header.payload.signature");
        assertFalse(bind.provisioning); assertEquals("header.payload.signature",bind.token);
        QrPayload provision=QrPayload.parse("{\"ver\":\"v1\",\"name\":\"ACHO_AABBCC\",\"pop\":\"abcd1234\",\"transport\":\"softap\"}");
        assertTrue(provision.provisioning); assertEquals("abcd1234",provision.password);
    }
    @Test public void rejectsForeignCodesAndUnsupportedSecurity() {
        for(String value:new String[]{"hello", "https://example.com", "{}", "{\"ver\":\"v1\",\"name\":\"ACHO_AABBCC\",\"pop\":\"abcd1234\",\"transport\":\"ble\"}", "{\"ver\":\"v1\",\"name\":\"ACHO_AABBCC\",\"pop\":\"abcd1234\",\"transport\":\"softap\",\"security\":2}"}) {
            try { QrPayload.parse(value); fail(value); } catch(IllegalArgumentException expected) {}
        }
    }
    @Test public void indexedImageHandlesOddWidthAndRoundTrips() {
        int[] pixels={0xff000000,0xffffffff,0xff000000,0xffffffff,0xff000000,0xffffffff};
        IndexedImage image=IndexedImage.dither(pixels,3,2);
        assertArrayEquals(new byte[]{7,12,64,0,0,0,0,(byte)255,(byte)255,(byte)255,(byte)255,(byte)255,64,(byte)160},image.binary);
        IndexedImage decoded=IndexedImage.decode(image.binary);
        assertArrayEquals(image.pixels,decoded.pixels);
        assertEquals(3,decoded.width); assertEquals(2,decoded.height);
    }
    @Test public void transparentPixelsBecomeWhite() {
        assertEquals(0xffffffff,IndexedImage.dither(new int[]{0},1,1).pixels[0]);
    }
    @Test public void corruptImagesAreRejected() {
        try { IndexedImage.decode(new byte[12]); fail(); } catch(IllegalArgumentException expected) {}
        try { IndexedImage.dither(new int[2],1,1); fail(); } catch(IllegalArgumentException expected) {}
    }
}
