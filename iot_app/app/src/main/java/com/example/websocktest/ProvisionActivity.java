package com.example.websocktest;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import androidx.core.content.ContextCompat;
import com.espressif.provisioning.DeviceConnectionEvent;
import com.espressif.provisioning.ESPConstants;
import com.espressif.provisioning.ESPDevice;
import com.espressif.provisioning.ESPProvisionManager;
import com.espressif.provisioning.WiFiAccessPoint;
import com.espressif.provisioning.listeners.ProvisionListener;
import com.espressif.provisioning.listeners.ResponseListener;
import com.espressif.provisioning.listeners.WiFiScanListener;
import com.example.websocktest.core.ServerAddress;
import com.example.websocktest.core.Ui;
import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;
import org.json.JSONException;
import org.json.JSONObject;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

/** Espressif Security 1; custom-data is sent before applying Wi-Fi credentials. */
public class ProvisionActivity extends BaseActivity {
    private final Handler main=new Handler(Looper.getMainLooper());
    private EditText deviceName,pop,apPassword,ssid,password,server;
    private Button discover,connect,scan,provision;
    private TextView status;
    private ESPDevice device;
    private boolean connected,busy,complete;
    private int permissionAction;
    private final Runnable timeout=() -> fail("操作超时，请确认设备仍在配网模式，或重启配网后重试");
    private final ActivityResultLauncher<String[]> permissions=registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),result -> {
        boolean allowed=true;
        for(boolean granted:result.values()) allowed &= granted;
        if(!allowed) { status.setText("发现和连接设备需要附近设备 / 位置权限。请在系统设置中允许后重试。"); return; }
        if(permissionAction==1) discoverDevices(); else connectDevice();
    });
    @Override protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        LinearLayout root=Ui.screen(this,"设备 Wi-Fi 配置");
        Ui.text(this,root,"首次启动或重置配网后，设备广播 ACHO_ 开头的热点。扫描设备配网码或使用 NFC 碰一碰可自动填入热点密码和 PoP；也可在设备配网信息页查看。配网完成后，再扫码或碰一碰绑定账号。");
        deviceName=Ui.input(this,root,"设备热点名称，例如 ACHO_123456",false);
        pop=Ui.input(this,root,"PoP 验证码",true);
        apPassword=Ui.input(this,root,"热点密码（留空时使用 PoP）",true);
        deviceName.setText(getIntent().getStringExtra("name"));
        pop.setText(getIntent().getStringExtra("pop")); apPassword.setText(getIntent().getStringExtra("password"));
        discover=Ui.button(this,root,"发现附近的 Acho 设备",v -> withPermissions(1));
        connect=Ui.button(this,root,"连接设备热点",v -> withPermissions(2));
        Ui.button(this,root,"打开系统 Wi-Fi 设置",v -> startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS)));
        Ui.text(this,root,"连接设备后，选择设备要加入的 2.4GHz 路由器：");
        ssid=Ui.input(this,root,"路由器 Wi-Fi 名称",false);
        password=Ui.input(this,root,"路由器密码（开放网络留空）",true);
        server=Ui.input(this,root,"Django 服务器地址",false); server.setText(settings.server());
        scan=Ui.button(this,root,"扫描设备附近的路由器",v -> scanRouters());
        provision=Ui.button(this,root,"保存服务器地址并开始配网",v -> provision());
        status=Ui.text(this,root,"请先连接设备热点；系统提示无互联网时，请保持连接。");
        Ui.button(this,root,"返回",v -> finish());
        EventBus.getDefault().register(this);
        updateButtons();
    }
    private void withPermissions(int action) {
        if(busy) return;
        permissionAction=action;
        ArrayList<String> needed=new ArrayList<>();
        if((action==1 || Build.VERSION.SDK_INT<33)
                && ContextCompat.checkSelfPermission(this,Manifest.permission.ACCESS_FINE_LOCATION)!=PackageManager.PERMISSION_GRANTED) {
            needed.add(Manifest.permission.ACCESS_FINE_LOCATION);
            needed.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }
        if(Build.VERSION.SDK_INT>=33 && ContextCompat.checkSelfPermission(this,Manifest.permission.NEARBY_WIFI_DEVICES)!=PackageManager.PERMISSION_GRANTED)
            needed.add(Manifest.permission.NEARBY_WIFI_DEVICES);
        if(needed.isEmpty()) { if(action==1) discoverDevices(); else connectDevice(); }
        else permissions.launch(needed.toArray(new String[0]));
    }
    private void discoverDevices() {
        busy=true; status.setText("正在发现 Acho 热点…若无结果，请打开系统定位和 Wi-Fi，或手动输入热点名。"); updateButtons();
        main.postDelayed(timeout,25000);
        try {
            ESPProvisionManager.getInstance(getApplicationContext()).searchWiFiEspDevices("ACHO_",new WiFiScanListener() {
                @Override public void onWifiListReceived(ArrayList<WiFiAccessPoint> networks) {
                    main.post(() -> {
                        if(isDestroyed()) return;
                        main.removeCallbacks(timeout); busy=false; updateButtons();
                        if(networks.isEmpty()) { status.setText("未发现设备，请确认配网模式已开启，也可手动输入热点名称。"); return; }
                        String[] names=new String[networks.size()];
                        for(int i=0;i<networks.size();i++) names[i]=networks.get(i).getWifiName();
                        new AlertDialog.Builder(ProvisionActivity.this).setTitle("选择 Acho 设备").setItems(names,(dialog,which) -> deviceName.setText(names[which])).show();
                        status.setText("选择设备后，输入 PoP 并连接热点。");
                    });
                }
                @Override public void onWiFiScanFailed(Exception error) { main.post(() -> fail("发现失败，请打开 Wi-Fi 和系统定位，或手动输入热点名称")); }
            });
        } catch(SecurityException | IllegalStateException error) { fail("无法扫描 Wi-Fi，请检查权限和系统定位开关"); }
    }
    private void connectDevice() {
        String name=deviceName.getText().toString().trim();
        String proof=pop.getText().toString(), key=apPassword.getText().toString();
        if(key.isEmpty()) key=proof;
        if(!name.startsWith("ACHO_") || name.getBytes(StandardCharsets.UTF_8).length>32 || proof.isEmpty()
                || key.length()<8 || key.length()>63) { status.setText("请输入正确的 ACHO_ 热点名、PoP 和热点密码（8～63 位）"); return; }
        releaseDevice(); complete=false;
        device=ESPProvisionManager.getInstance(getApplicationContext()).createESPDevice(
                ESPConstants.TransportType.TRANSPORT_SOFTAP,ESPConstants.SecurityType.SECURITY_1);
        device.setProofOfPossession(proof);
        busy=true; updateButtons(); status.setText("请在系统对话框中允许连接设备热点，并保持连接…"); main.postDelayed(timeout,60000);
        try { device.connectWiFiDevice(name,key); }
        catch(SecurityException | IllegalArgumentException error) { fail("连接失败，请检查权限、热点密码和 Wi-Fi 状态"); }
    }
    @Subscribe(threadMode=ThreadMode.MAIN)
    public void onDeviceConnection(DeviceConnectionEvent event) {
        if(device==null || complete) return;
        if(event.getEventType()==ESPConstants.EVENT_DEVICE_CONNECTED) {
            main.removeCallbacks(timeout); connected=true; busy=false; updateButtons();
            status.setText("已连接设备。请设置路由器和服务器地址，再开始配网。");
        } else if(event.getEventType()==ESPConstants.EVENT_DEVICE_CONNECTION_FAILED)
            fail("热点连接失败，请确认热点名和密码，允许系统连接请求后重试");
        else if(event.getEventType()==ESPConstants.EVENT_DEVICE_DISCONNECTED && !busy)
            fail("设备连接已断开，请重新连接热点");
    }
    private void scanRouters() {
        if(!connected || busy) return;
        busy=true; updateButtons(); status.setText("设备正在扫描路由器…"); main.postDelayed(timeout,30000);
        device.scanNetworks(new WiFiScanListener() {
            @Override public void onWifiListReceived(ArrayList<WiFiAccessPoint> networks) {
                main.post(() -> {
                    if(isDestroyed()) return;
                    main.removeCallbacks(timeout); busy=false; updateButtons();
                    String[] names=new String[networks.size()];
                    for(int i=0;i<networks.size();i++) names[i]=networks.get(i).getWifiName();
                    if(names.length==0) { status.setText("没有发现路由器，可手动输入 2.4GHz Wi-Fi 名称"); return; }
                    new AlertDialog.Builder(ProvisionActivity.this).setTitle("选择路由器 Wi-Fi").setItems(names,(dialog,which) -> ssid.setText(names[which])).show();
                    status.setText("请选择路由器并输入密码。");
                });
            }
            @Override public void onWiFiScanFailed(Exception error) { main.post(() -> fail("扫描路由器失败，请确认 PoP 正确并重新连接设备")); }
        });
    }
    private void provision() {
        if(!connected || busy) return;
        String network=ssid.getText().toString(), secret=password.getText().toString();
        int bytes=secret.getBytes(StandardCharsets.UTF_8).length;
        if(network.isEmpty() || network.getBytes(StandardCharsets.UTF_8).length>32 || (bytes!=0 && (bytes<8 || bytes>63))) {
            status.setText("Wi-Fi 名称需要 1～32 字节；密码为 8～63 字节，开放网络请留空"); return;
        }
        final String address;
        try { address=ServerAddress.normalize(server.getText().toString()); }
        catch(IllegalArgumentException error) { server.setError(error.getMessage()); return; }
        busy=true; updateButtons(); main.postDelayed(timeout,90000);
        status.setText("正在验证 PoP 并保存服务器地址…");
        final byte[] data;
        try { data=new JSONObject().put("server_url",address).toString().getBytes(StandardCharsets.UTF_8); }
        catch(JSONException error) { fail("服务器地址无效"); return; }
        device.sendDataToCustomEndPoint("custom-data",data,new ResponseListener() {
            @Override public void onSuccess(byte[] response) {
                main.post(() -> {
                    if(isDestroyed() || device==null) return;
                    try {
                        JSONObject result=new JSONObject(new String(response,StandardCharsets.UTF_8));
                        if(!"successful".equals(result.optString("status"))) { fail("设备未接受服务器地址，请检查固件版本和地址格式"); return; }
                        settings.setServer(address);
                        status.setText("服务器地址已保存，正在配置路由器…");
                        device.provision(network,secret,listener);
                    } catch(JSONException error) { fail("设备返回的数据格式错误，请检查固件版本"); }
                });
            }
            @Override public void onFailure(Exception error) { main.post(() -> fail("PoP 验证或服务器地址保存失败，请确认验证码与固件版本")); }
        });
    }
    private final ProvisionListener listener=new ProvisionListener() {
        @Override public void createSessionFailed(Exception error) { main.post(() -> fail("PoP 验证失败，请重新连接并检查验证码")); }
        @Override public void wifiConfigSent() { showState("Wi-Fi 配置已发送，等待设备应用配置…"); }
        @Override public void wifiConfigFailed(Exception error) { main.post(() -> fail("Wi-Fi 配置发送失败，请重新连接设备")); }
        @Override public void wifiConfigApplied() { showState("设备正在连接路由器并获取 IP…"); }
        @Override public void wifiConfigApplyFailed(Exception error) { main.post(() -> fail("设备应用配置失败，请重新连接后重试")); }
        @Override public void provisioningFailedFromDevice(ESPConstants.ProvisionFailureReason reason) {
            main.post(() -> fail(reason==ESPConstants.ProvisionFailureReason.AUTH_FAILED?"路由器密码错误，请重新连接设备后重试":
                    reason==ESPConstants.ProvisionFailureReason.NETWORK_NOT_FOUND?"设备找不到该路由器，请确认开启了 2.4GHz Wi-Fi":"设备配网失败，请重新连接后重试"));
        }
        @Override public void deviceProvisioningSuccess() {
            main.post(() -> {
                if(isDestroyed()) return;
                main.removeCallbacks(timeout); complete=true; busy=false; releaseDevice(); updateButtons();
                status.setText("配网成功！手机已解除设备热点网络绑定。请恢复普通网络，再在设备进入 NFC Tap → NFC Account Pair，用 App 碰一碰绑定；也可进入 Bind 扫码。");
            });
        }
        @Override public void onProvisioningFailed(Exception error) { main.post(() -> fail("配网未确认成功，请查看设备状态；如未连接路由器，可重试配网")); }
    };
    private void showState(String text) { main.post(() -> { if(!isDestroyed()) status.setText(text); }); }
    private void fail(String text) {
        if(isDestroyed() || complete) return;
        main.removeCallbacks(timeout); busy=false; releaseDevice(); updateButtons(); status.setText(text);
    }
    private void updateButtons() {
        discover.setEnabled(!busy); connect.setEnabled(!busy);
        scan.setEnabled(connected&&!busy); provision.setEnabled(connected&&!busy);
        deviceName.setEnabled(!busy); pop.setEnabled(!busy); apPassword.setEnabled(!busy);
        ssid.setEnabled(!busy); password.setEnabled(!busy); server.setEnabled(!busy);
    }
    private void releaseDevice() {
        connected=false;
        ESPDevice previous=device; device=null;
        if(previous!=null) previous.disconnectDevice();
    }
    @Override protected void onDestroy() {
        EventBus.getDefault().unregister(this); main.removeCallbacksAndMessages(null); releaseDevice(); super.onDestroy();
    }
}
