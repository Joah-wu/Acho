package com.example.websocktest.core;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.Closeable;
import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.FormBody;
import okhttp3.HttpUrl;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/** Parses responses off the UI thread, owns calls per Activity, always closes bodies. */
public final class ApiClient implements Closeable {
    public interface Result { void success(JSONObject data); void failure(String message); }
    public static final OkHttpClient HTTP = new OkHttpClient.Builder()
            .connectTimeout(10,TimeUnit.SECONDS).readTimeout(30,TimeUnit.SECONDS)
            .writeTimeout(60,TimeUnit.SECONDS).callTimeout(120,TimeUnit.SECONDS)
            .followRedirects(false).build();
    private final AppSettings settings;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final Set<Call> calls = Collections.synchronizedSet(new HashSet<>());
    private volatile boolean closed;
    public ApiClient(AppSettings settings) { this.settings=settings; }
    public HttpUrl url(String path, Map<String,String> parameters, boolean authenticated) {
        HttpUrl base=HttpUrl.parse(settings.server());
        if (base==null) throw new IllegalArgumentException("请先配置服务器地址");
        HttpUrl.Builder builder=base.newBuilder().encodedPath(path);
        for (Map.Entry<String,String> entry:parameters.entrySet()) builder.addQueryParameter(entry.getKey(),entry.getValue());
        if (authenticated) builder.addQueryParameter("tok",settings.token());
        return builder.build();
    }
    public void get(String path, Map<String,String> parameters, boolean auth, Result result) {
        execute(new Request.Builder().url(url(path,parameters,auth)).get().build(),auth,result);
    }
    public void post(String path, Map<String,String> parameters, Result result) {
        FormBody.Builder body=new FormBody.Builder();
        for (Map.Entry<String,String> entry:parameters.entrySet()) body.add(entry.getKey(),entry.getValue());
        postBody(path,body.build(),false,result);
    }
    public void postBody(String path, RequestBody body, boolean auth, Result result) {
        execute(new Request.Builder().url(url(path,Collections.emptyMap(),false)).post(body).build(),auth,result);
    }
    private void execute(Request request, boolean auth, Result result) {
        if (closed) return;
        if (auth && !settings.loggedIn()) { result.failure("请先登录"); return; }
        final String token=settings.token(), server=settings.server();
        Call call=HTTP.newCall(request); calls.add(call);
        call.enqueue(new Callback() {
            @Override public void onFailure(@NonNull Call call,@NonNull IOException error) {
                calls.remove(call);
                deliver(() -> result.failure("网络请求失败，请检查服务器地址和网络连接"));
            }
            @Override public void onResponse(@NonNull Call call,@NonNull Response response) {
                calls.remove(call);
                try (Response owned=response) {
                    if (owned.body()==null) throw new IOException("empty body");
                    JSONObject data=new JSONObject(owned.body().string());
                    if (!server.equals(settings.server())) return;
                    String state=data.optString("status");
                    boolean expired=auth && "failed".equals(state)
                            && ("expired".equals(data.optString("token")) || "error".equals(data.optString("token")));
                    if (expired) {
                        deliver(() -> { if (settings.token().equals(token)) settings.logout(); result.failure("登录已过期，请重新登录"); });
                    } else if (!owned.isSuccessful()) {
                        deliver(() -> result.failure(data.optString("message","请求失败（"+owned.code()+"）")));
                    } else {
                        deliver(() -> {
                            if (auth && data.optString("token").contains(".")) settings.renew(token,data.optString("token"));
                            result.success(data);
                        });
                    }
                } catch (IOException | JSONException error) {
                    deliver(() -> result.failure("服务器响应异常，请确认使用正确的 Django 服务地址"));
                }
            }
        });
    }
    private void deliver(Runnable action) { main.post(() -> { if (!closed) action.run(); }); }
    @Override public void close() {
        closed=true;
        synchronized (calls) { for (Call call:calls) call.cancel(); calls.clear(); }
        main.removeCallbacksAndMessages(null);
    }
}
