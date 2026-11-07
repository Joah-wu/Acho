package com.example.websocktest.core;

import org.json.JSONException;
import org.json.JSONObject;

public final class QrPayload {
    public final boolean provisioning;
    public final String token, name, pop, password;
    private QrPayload(boolean provisioning, String token, String name, String pop, String password) {
        this.provisioning=provisioning; this.token=token; this.name=name; this.pop=pop; this.password=password;
    }
    public static QrPayload parse(String raw) {
        String text = raw == null ? "" : raw.trim();
        if (text.isEmpty() || text.length() > 2048) throw new IllegalArgumentException("二维码内容无效");
        if (text.startsWith("{")) {
            try {
                JSONObject json = new JSONObject(text);
                String name=json.optString("name"), pop=json.optString("pop");
                if (!"v1".equals(json.optString("ver")) || !"softap".equals(json.optString("transport"))
                        || !name.startsWith("ACHO_") || name.length()>32 || pop.isEmpty()
                        || json.optInt("security", 1)!=1) throw new IllegalArgumentException("不支持此设备配网码");
                return new QrPayload(true,"",name,pop,json.optString("password",pop));
            } catch (JSONException e) { throw new IllegalArgumentException("配网二维码格式不正确"); }
        }
        // Django's existing timed device binding token has three URL-safe segments.
        if (!text.matches("[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+"))
            throw new IllegalArgumentException("请扫描 Acho 设备绑定二维码或配网二维码");
        return new QrPayload(false,text,"","","");
    }
}
