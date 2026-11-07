package com.example.websocktest.core;

import android.content.Context;
import android.content.SharedPreferences;

public final class AppSettings {
    private final SharedPreferences preferences;
    public AppSettings(Context context) {
        preferences = context.getApplicationContext().getSharedPreferences("acho_session", Context.MODE_PRIVATE);
    }
    public String server() { return preferences.getString("server", "http://192.168.1.100:8787"); }
    public void setServer(String value) {
        String server = ServerAddress.normalize(value);
        if (!server.equals(server())) {
            preferences.edit().putString("server", server).remove("token").remove("username").apply();
        }
    }
    public String token() { return preferences.getString("token", ""); }
    public String username() { return preferences.getString("username", ""); }
    public boolean loggedIn() { return !token().isEmpty(); }
    public void login(String name, String token) {
        preferences.edit().putString("username", name).putString("token", token).apply();
    }
    public void renew(String previousToken, String renewed) {
        if (!renewed.isEmpty() && token().equals(previousToken)) preferences.edit().putString("token", renewed).apply();
    }
    public void logout() { preferences.edit().remove("username").remove("token").apply(); }
}
