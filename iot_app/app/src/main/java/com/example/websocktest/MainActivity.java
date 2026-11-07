package com.example.websocktest;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import com.example.websocktest.core.ApiClient;
import com.example.websocktest.core.QrPayload;
import com.example.websocktest.core.Ui;
import com.journeyapps.barcodescanner.ScanContract;
import com.journeyapps.barcodescanner.ScanOptions;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class MainActivity extends BaseActivity {
    private TextView account, status, server;
    private LinearLayout devices;
    private Button refresh;
    private String bindingToken="";
    private final ActivityResultLauncher<ScanOptions> scanner=registerForActivityResult(new ScanContract(),result -> {
        if(result.getContents()!=null) handleQr(result.getContents());
    });
    private final ActivityResultLauncher<Intent> nfc=registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),result -> {
        if(result.getResultCode()==RESULT_OK && result.getData()!=null) consumeNfc(result.getData());
    });
    @Override protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        if(saved!=null) bindingToken=saved.getString("binding_token","");
        LinearLayout root=Ui.screen(this,"Acho 设备");
        server=Ui.text(this,root,"");
        Ui.button(this,root,"设置服务器地址",v -> configureServer());
        account=Ui.text(this,root,"");
        Ui.button(this,root,"登录 / 注册",v -> startActivity(new Intent(this,UserActivity.class)));
        Ui.button(this,root,"退出登录",v -> { settings.logout(); bindingToken=""; refreshDevices(); });
        Ui.button(this,root,"给设备配置 Wi-Fi",v -> startActivity(new Intent(this,ProvisionActivity.class)));
        Ui.button(this,root,"NFC 碰一碰配网 / 绑定",v -> nfc.launch(new Intent(this,NfcActivity.class).putExtra(NfcActivity.EXTRA_RETURN,true)));
        Ui.button(this,root,"扫码绑定 / 扫描配网码",v -> {
            ScanOptions options=new ScanOptions().setDesiredBarcodeFormats(ScanOptions.QR_CODE)
                    .setPrompt("扫描设备绑定码或 Acho 配网码").setBeepEnabled(false).setOrientationLocked(false);
            scanner.launch(options);
        });
        Ui.button(this,root,"粘贴绑定码 / 配网码",v -> {
            EditText input=new EditText(this); input.setHint("粘贴二维码原始内容");
            new AlertDialog.Builder(this).setTitle("添加设备").setView(input)
                    .setNegativeButton("取消",null).setPositiveButton("继续",(dialog,which) -> handleQr(input.getText().toString())).show();
        });
        Ui.button(this,root,"本地图片工具",v -> startActivity(new Intent(this,CameraAlbumTest.class)));
        refresh=Ui.button(this,root,"刷新我的设备",v -> refreshDevices());
        status=Ui.text(this,root,"");
        devices=new LinearLayout(this); devices.setOrientation(LinearLayout.VERTICAL); root.addView(devices);
        consumeNfc(getIntent());
    }
    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent); setIntent(intent); consumeNfc(intent);
    }
    private void consumeNfc(Intent intent) {
        String payload=intent.getStringExtra(NfcActivity.EXTRA_PAYLOAD);
        intent.removeExtra(NfcActivity.EXTRA_PAYLOAD);
        if(payload!=null) handleQr(payload);
    }
    @Override protected void onResume() {
        super.onResume();
        if(!bindingToken.isEmpty() && settings.loggedIn()) bindDevice(); else refreshDevices();
    }
    private void configureServer() {
        EditText input=new EditText(this); input.setSingleLine(true); input.setText(settings.server());
        AlertDialog dialog=new AlertDialog.Builder(this).setTitle("Django 服务器地址").setView(input)
                .setNegativeButton("取消",null).setPositiveButton("保存",null).create();
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
            try {
                settings.setServer(input.getText().toString());
                api.close(); api=new ApiClient(settings); bindingToken="";
                dialog.dismiss(); refreshDevices();
            } catch(IllegalArgumentException error) { input.setError(error.getMessage()); }
        }));
        dialog.show();
    }
    private void handleQr(String content) {
        try {
            QrPayload qr=QrPayload.parse(content);
            if(qr.provisioning) {
                startActivity(new Intent(this,ProvisionActivity.class).putExtra("name",qr.name)
                        .putExtra("pop",qr.pop).putExtra("password",qr.password));
            } else {
                bindingToken=qr.token;
                if(!settings.loggedIn()) {
                    message("请先登录，登录完成后将继续绑定");
                    startActivity(new Intent(this,UserActivity.class));
                } else bindDevice();
            }
        } catch(IllegalArgumentException error) { message(error.getMessage()); }
    }
    private void bindDevice() {
        String token=bindingToken; bindingToken="";
        status.setText("正在绑定设备…");
        Map<String,String> values=new HashMap<>(); values.put("bind_token",token);
        api.get("/home/user/bind/",values,true,new ApiClient.Result() {
            @Override public void success(JSONObject data) {
                switch(data.optString("status")) {
                    case "successful": message("设备绑定成功"); refreshDevices(); break;
                    case "existed": message("此设备已在你的设备列表中"); refreshDevices(); break;
                    case "QR-expired": status.setText("绑定码已过期，请在设备上重新进入绑定页，再扫码或碰一碰"); break;
                    default: status.setText("绑定失败，请重新登录后扫码");
                }
            }
            @Override public void failure(String text) { status.setText(text); }
        });
    }
    private void refreshDevices() {
        server.setText("服务器："+settings.server());
        account.setText(settings.loggedIn()?"已登录："+settings.username():"尚未登录");
        devices.removeAllViews();
        if(!settings.loggedIn()) { refresh.setEnabled(true); status.setText("登录后可查看设备。新设备请先配网，再扫码或 NFC 碰一碰绑定。"); return; }
        refresh.setEnabled(false); status.setText("正在获取设备…");
        api.get("/home/user/getdevices_data/",Collections.emptyMap(),true,new ApiClient.Result() {
            @Override public void success(JSONObject data) {
                refresh.setEnabled(true);
                JSONArray list=data.optJSONArray("data");
                if(!"successful".equals(data.optString("status")) || list==null) { status.setText("设备列表加载失败，请重新登录"); return; }
                status.setText(list.length()==0?"还没有绑定设备。请选择扫码或 NFC 绑定。":"点击设备管理图片与音频；在线状态以服务器返回值为准。");
                devices.removeAllViews();
                for(int i=0;i<list.length();i++) {
                    JSONObject device=list.optJSONObject(i); if(device==null) continue;
                    String name=device.optString("nick_name","Acho");
                    String online="online".equals(device.optString("status"))?"在线":"离线";
                    Ui.button(MainActivity.this,devices,name+" · "+online+"\n"+device.optString("mac"),v ->
                            startActivity(new Intent(MainActivity.this,DeviceFilesActivity.class)
                                    .putExtra("device_id",device.optLong("id")).putExtra("device_name",name)));
                }
            }
            @Override public void failure(String text) { refresh.setEnabled(true); status.setText(text); }
        });
    }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putString("binding_token",bindingToken); super.onSaveInstanceState(state);
    }
}
