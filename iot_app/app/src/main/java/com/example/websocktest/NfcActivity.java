package com.example.websocktest;

import android.content.Intent;
import android.nfc.NfcAdapter;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.tech.NfcV;
import android.nfc.tech.TagTechnology;
import android.os.Bundle;
import android.os.Parcelable;
import android.provider.Settings;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import com.example.websocktest.core.NfcPayload;
import com.example.websocktest.core.QrPayload;
import com.example.websocktest.core.Ui;
import org.json.JSONObject;
import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicBoolean;

public class NfcActivity extends AppCompatActivity implements NfcAdapter.ReaderCallback {
    public static final String EXTRA_PAYLOAD="acho_nfc_payload", EXTRA_RETURN="acho_nfc_return";
    private NfcAdapter adapter;
    private TextView status;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private final AtomicBoolean reading=new AtomicBoolean();
    private volatile TagTechnology connection;
    private volatile int generation;
    private volatile boolean resumed;
    private boolean initialIntent=true;
    @Override protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        LinearLayout root=Ui.screen(this,"NFC 碰一碰");
        Ui.text(this,root,"① 设备菜单 → NFC Tap → NFC Wi-Fi Setup，等待就绪后，将手机背面的 NFC 区域靠近设备天线。手机会自动读取配网信息，无需输入 PoP。\n\n② 配网完成、设备联网后，选择 NFC Account Pair，等设备显示 Ready，再碰一次绑定当前登录账号。");
        status=Ui.text(this,root,"");
        Ui.button(this,root,"打开 NFC 设置",v -> startActivity(new Intent(Settings.ACTION_NFC_SETTINGS)));
        Ui.button(this,root,"返回",v -> finish());
        adapter=NfcAdapter.getDefaultAdapter(this);
    }
    @Override protected void onResume() {
        super.onResume(); resumed=true;
        if(adapter==null) { status.setText("此手机不支持 NFC，请使用扫码配网与绑定。"); return; }
        if(!adapter.isEnabled()) { status.setText("请先开启手机 NFC，然后返回此页。"); return; }
        status.setText("等设备显示就绪后靠近天线，读取期间保持手机不动。");
        Bundle options=new Bundle(); options.putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY,250);
        adapter.enableReaderMode(this,this,NfcAdapter.FLAG_READER_NFC_V,options);
        if(initialIntent) { initialIntent=false; readIntent(getIntent()); }
    }
    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent); setIntent(intent); readIntent(intent);
    }
    private void readIntent(Intent intent) {
        if(!NfcAdapter.ACTION_NDEF_DISCOVERED.equals(intent.getAction())) return;
        Parcelable[] messages=intent.getParcelableArrayExtra(NfcAdapter.EXTRA_NDEF_MESSAGES);
        if(messages!=null && messages.length==1 && messages[0] instanceof NdefMessage) {
            try { deliver(NfcPayload.decodeMessage(((NdefMessage)messages[0]).toByteArray())); }
            catch(IllegalArgumentException error) { status.setText(error.getMessage()); }
            return;
        }
        Tag tag=intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
        if(tag!=null) onTagDiscovered(tag);
    }
    @Override public void onTagDiscovered(Tag tag) {
        if(!resumed || !reading.compareAndSet(false,true)) return;
        int current=generation;
        try { worker.execute(() -> {
            String payload=null, failure=null;
            try {
                Ndef ndef=Ndef.get(tag);
                if(ndef!=null) {
                    try {
                        connection=ndef; ndef.connect();
                        NdefMessage message=ndef.getNdefMessage();
                        if(message!=null) payload=NfcPayload.decodeMessage(message.toByteArray());
                    } catch(IOException | FormatException ignored) {
                        // Some Android versions cannot map an extended Type 5 CC; use NFC-V.
                    } finally { closeConnection(); }
                }
                if(payload==null) payload=NfcPayload.decodeTag(readRaw(tag,current));
            } catch(IllegalArgumentException error) { failure=error.getMessage(); }
            catch(Exception error) { failure="读取中断，请移开手机，等设备显示 Ready 后重新靠近天线。"; }
            finally { closeConnection(); reading.set(false); }
            String value=payload, error=failure;
            runOnUiThread(() -> {
                if(!resumed || current!=generation || isFinishing()) return;
                if(error!=null) status.setText(error); else deliver(value);
            });
        }); } catch(RejectedExecutionException ignored) { reading.set(false); }
    }
    private byte[] readRaw(Tag tag,int current) throws IOException {
        if(current!=generation) throw new IOException("Reading cancelled");
        NfcV nfc=NfcV.get(tag);
        if(nfc==null || tag.getId().length!=8) throw new IOException("NFC-V required");
        connection=nfc; nfc.connect();
        byte[] memory=new byte[NfcPayload.MEMORY_SIZE];
        // Addressed ISO15693 single block reads; this device reserves the first 128 x 4 bytes.
        byte[] command=new byte[11]; command[0]=0x22; command[1]=0x20;
        System.arraycopy(tag.getId(),0,command,2,8);
        for(int block=0;block<memory.length/4;block++) {
            if(current!=generation || Thread.currentThread().isInterrupted()) throw new IOException("Reading cancelled");
            command[10]=(byte)block;
            byte[] response=nfc.transceive(command);
            if(response==null || response.length!=5 || (response[0]&1)!=0) throw new IOException("Block read failed");
            System.arraycopy(response,1,memory,block*4,4);
            if(block==0 && memory[0]!=(byte)0xe1 && memory[0]!=(byte)0xe2) throw new IllegalArgumentException("这不是已准备好的 Acho NFC 标签，请先在设备菜单选择功能。");
        }
        return memory;
    }
    private void deliver(String payload) {
        try {
            if(payload!=null && payload.startsWith("{") && "idle".equals(new JSONObject(payload).optString("purpose"))) {
                status.setText("已发现 Acho。请先在设备 NFC 菜单选择 Wi-Fi Setup 或 Account Pair，移开手机，待设备就绪后再碰一次。"); return;
            }
            QrPayload.parse(payload);
            if(getIntent().getBooleanExtra(EXTRA_RETURN,false)) {
                setResult(RESULT_OK,new Intent().putExtra(EXTRA_PAYLOAD,payload));
            } else startActivity(new Intent(this,MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP)
                    .putExtra(EXTRA_PAYLOAD,payload));
            finish();
        } catch(Exception error) { status.setText(error.getMessage()==null?"NFC 内容无效，请重新选择设备菜单后再试。":error.getMessage()); }
    }
    private void closeConnection() {
        TagTechnology active=connection; connection=null;
        if(active!=null) try { active.close(); } catch(IOException ignored) { }
    }
    @Override protected void onPause() {
        resumed=false; generation++;
        if(adapter!=null) adapter.disableReaderMode(this);
        closeConnection(); super.onPause();
    }
    @Override protected void onDestroy() { worker.shutdownNow(); closeConnection(); super.onDestroy(); }
}
