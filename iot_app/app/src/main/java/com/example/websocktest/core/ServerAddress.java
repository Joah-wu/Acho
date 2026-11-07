package com.example.websocktest.core;

import java.net.URI;
import java.net.URISyntaxException;

/** Same origin format accepted by the ESP32 custom-data endpoint. */
public final class ServerAddress {
    private ServerAddress() {}
    public static String normalize(String input) {
        String value = input == null ? "" : input.trim();
        try {
            URI uri = new URI(value);
            if (!("http".equalsIgnoreCase(uri.getScheme()) || "https".equalsIgnoreCase(uri.getScheme()))
                    || uri.getHost() == null || uri.getRawUserInfo() != null
                    || uri.getRawQuery() != null || uri.getRawFragment() != null
                    || !(uri.getRawPath().isEmpty() || "/".equals(uri.getRawPath()))
                    || uri.getPort() == 0 || uri.getPort() > 65535) {
                throw new IllegalArgumentException("请输入服务器地址，例如 http://192.168.1.50:8787");
            }
            return new URI(uri.getScheme().toLowerCase(java.util.Locale.ROOT), null,
                    uri.getHost(), uri.getPort(), null, null, null).toString();
        } catch (URISyntaxException e) {
            throw new IllegalArgumentException("服务器地址格式不正确");
        }
    }
}
