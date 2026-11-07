package com.example.websocktest;

import android.net.Uri;
import android.Manifest;
import android.content.pm.PackageManager;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.database.Cursor;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AlertDialog;
import com.example.websocktest.core.ApiClient;
import com.example.websocktest.core.Ui;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.IOException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import okhttp3.Call;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class DeviceFilesActivity extends BaseActivity {
    private static final long MAX_FILE=32L*1024*1024;
    private long deviceId;
    private int fileType=1, pendingUploadType=1;
    private TextView status;
    private LinearLayout files;
    private Button upload,refresh,images,audio,capture;
    private File cameraPhoto;
    private JSONObject pendingDownload;
    private final ExecutorService downloads=Executors.newSingleThreadExecutor();
    private final ExecutorService uploads=Executors.newSingleThreadExecutor();
    private volatile File stagedUpload;
    private volatile Call downloadCall;
    private boolean uploading;
    private final ActivityResultLauncher<Uri> camera=registerForActivityResult(new ActivityResultContracts.TakePicture(),ok -> {
        if(ok && cameraPhoto!=null) {
            pendingUploadType=1;
            describeUpload(FileProvider.getUriForFile(this,getPackageName()+".fileprovider",cameraPhoto));
        }
    });
    private final ActivityResultLauncher<String> cameraPermission=registerForActivityResult(new ActivityResultContracts.RequestPermission(),allowed -> {
        if(allowed) takePhoto(); else message("拍照需要相机权限，也可以选择已有图片上传");
    });
    private final ActivityResultLauncher<String[]> picker=registerForActivityResult(new ActivityResultContracts.OpenDocument(),uri -> {
        if(uri!=null) describeUpload(uri);
    });
    private final ActivityResultLauncher<String> saveFile=registerForActivityResult(new ActivityResultContracts.CreateDocument("application/octet-stream"),uri -> {
        if(uri!=null && pendingDownload!=null) download(uri,pendingDownload);
        pendingDownload=null;
    });
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state); deviceId=getIntent().getLongExtra("device_id",0);
        if(deviceId<=0) { finish(); return; }
        if(state!=null) {
            fileType=state.getInt("type",1); pendingUploadType=state.getInt("upload_type",1);
            if(state.containsKey("camera_photo")) cameraPhoto=new File(state.getString("camera_photo"));
            try { if(state.containsKey("download")) pendingDownload=new JSONObject(state.getString("download")); }
            catch(JSONException ignored) { pendingDownload=null; }
        }
        LinearLayout root=Ui.screen(this,getIntent().getStringExtra("device_name"));
        Ui.text(this,root,"上传到服务器后，请在设备端选择 Get List，再进入 Download 下载。音频使用设备支持的 PCM WAV 格式。");
        images=Ui.button(this,root,"图片",v -> { fileType=1; load(); });
        audio=Ui.button(this,root,"音频",v -> { fileType=2; load(); });
        upload=Ui.button(this,root,"上传文件",v -> {
            pendingUploadType=fileType;
            picker.launch(fileType==1?new String[]{"image/jpeg","image/png","image/webp","image/bmp"}:new String[]{"audio/*","application/octet-stream"});
        });
        capture=Ui.button(this,root,"拍照上传图片",v -> {
            if(ContextCompat.checkSelfPermission(this,Manifest.permission.CAMERA)==PackageManager.PERMISSION_GRANTED) takePhoto();
            else cameraPermission.launch(Manifest.permission.CAMERA);
        });
        refresh=Ui.button(this,root,"刷新列表",v -> load());
        status=Ui.text(this,root,"");
        files=new LinearLayout(this); files.setOrientation(LinearLayout.VERTICAL); root.addView(files);
        Ui.button(this,root,"返回设备列表",v -> finish());
        load();
    }
    private Map<String,String> deviceParameters() {
        Map<String,String> p=new HashMap<>(); p.put("dev_id",Long.toString(deviceId)); return p;
    }
    private void load() {
        if(uploading) return;
        int requestedType=fileType;
        images.setEnabled(fileType!=1); audio.setEnabled(fileType!=2); capture.setEnabled(fileType==1);
        refresh.setEnabled(false); files.removeAllViews(); status.setText("正在加载文件列表…");
        Map<String,String> p=deviceParameters(); p.put("file_type",Integer.toString(fileType));
        api.get("/acho/api/files/",p,true,new ApiClient.Result() {
            @Override public void success(JSONObject response) {
                if(requestedType!=fileType || uploading) return;
                refresh.setEnabled(true);
                JSONArray list=response.optJSONArray("data");
                if(!"successful".equals(response.optString("status")) || list==null) { status.setText(response.optString("message","无法获取文件列表")); return; }
                files.removeAllViews(); status.setText(list.length()==0?"暂无文件，点击上传添加。":"共 "+list.length()+" 个"+(fileType==1?"图片":"音频")+"文件");
                for(int i=0;i<list.length();i++) {
                    JSONObject file=list.optJSONObject(i); if(file==null) continue;
                    Ui.text(DeviceFilesActivity.this,files,file.optString("name")+"\n"+file.optString("description")+"\n"+file.optLong("size",0)+" 字节");
                    Ui.button(DeviceFilesActivity.this,files,"下载到手机",v -> { pendingDownload=file; saveFile.launch(file.optString("name","acho-file")); });
                    Ui.button(DeviceFilesActivity.this,files,"删除",v -> new AlertDialog.Builder(DeviceFilesActivity.this)
                            .setTitle("删除文件？").setMessage(file.optString("name"))
                            .setNegativeButton("取消",null).setPositiveButton("删除",(dialog,which) -> remove(file)).show());
                }
            }
            @Override public void failure(String text) { if(requestedType==fileType && !uploading) { refresh.setEnabled(true); status.setText(text); } }
        });
    }
    private void takePhoto() {
        if(uploading) return;
        try {
            if(cameraPhoto!=null) cameraPhoto.delete();
            cameraPhoto=File.createTempFile("acho-photo-",".jpg",getCacheDir());
            camera.launch(FileProvider.getUriForFile(this,getPackageName()+".fileprovider",cameraPhoto));
        } catch(IOException | RuntimeException error) { message("无法打开相机，请检查相机应用与权限"); }
    }
    private void describeUpload(Uri uri) {
        String name=""; long size=-1;
        try(Cursor cursor=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE},null,null,null)) {
            if(cursor!=null && cursor.moveToFirst()) {
                int nameColumn=cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                int sizeColumn=cursor.getColumnIndex(OpenableColumns.SIZE);
                if(nameColumn>=0) name=cursor.getString(nameColumn);
                if(sizeColumn>=0 && !cursor.isNull(sizeColumn)) size=cursor.getLong(sizeColumn);
            }
        } catch(RuntimeException error) { status.setText("无法读取所选文件，请重新选择"); return; }
        if(name==null || name.isEmpty()) { status.setText("无法读取文件名，请重新选择"); return; }
        if(size>MAX_FILE || size==0) { status.setText("请选择 0～32 MiB 范围内的非空文件"); return; }
        if(pendingUploadType==2 && !name.toLowerCase(java.util.Locale.ROOT).endsWith(".wav")) {
            status.setText("设备播放器使用 WAV，请先将音频转换为 PCM WAV 文件"); return;
        }
        EditText description=new EditText(this); description.setHint("文件说明（可选，最多 200 字）");
        final String filename=name; final int type=pendingUploadType;
        new AlertDialog.Builder(this).setTitle("上传 "+name).setView(description).setNegativeButton("取消",null)
                .setPositiveButton("上传",(dialog,which) -> upload(uri,filename,type,description.getText().toString())).show();
    }
    private void upload(Uri uri,String name,int type,String description) {
        if(description.length()>200) { status.setText("文件说明不能超过 200 字"); return; }
        uploading=true; upload.setEnabled(false); capture.setEnabled(false); refresh.setEnabled(false); images.setEnabled(false); audio.setEnabled(false);
        status.setText("正在准备上传 "+name+"…");
        // Cloud document providers may omit SIZE. Snapshot to a bounded cache file
        // so multipart always has Content-Length, as Django's parser requires.
        uploads.execute(() -> {
            File staged=null;
            try {
                staged=File.createTempFile("acho-upload-",".tmp",getCacheDir());
                stagedUpload=staged;
                try(InputStream input=getContentResolver().openInputStream(uri); OutputStream output=new FileOutputStream(staged)) {
                    if(input==null) throw new IOException("File unavailable");
                    byte[] buffer=new byte[8192]; int n; long total=0;
                    while((n=input.read(buffer))!=-1) {
                        if(Thread.currentThread().isInterrupted()) throw new IOException("Cancelled");
                        total+=n; if(total>MAX_FILE) throw new IOException("File too large");
                        output.write(buffer,0,n);
                    }
                    if(total==0) throw new IOException("Empty file");
                }
                final File ready=staged;
                String mime=getContentResolver().getType(uri);
                RequestBody stream=RequestBody.create(MediaType.parse(mime==null?"application/octet-stream":mime),ready);
                MultipartBody body=new MultipartBody.Builder().setType(MultipartBody.FORM)
                        .addFormDataPart("tok",settings.token()).addFormDataPart("device_id",Long.toString(deviceId))
                        .addFormDataPart("type",Integer.toString(type)).addFormDataPart("description",description)
                        .addFormDataPart("file",name,stream).build();
                runOnUiThread(() -> {
                    if(isDestroyed()) { ready.delete(); return; }
                    status.setText("正在上传 "+name+"…");
                    api.postBody("/acho/api/files/",body,true,new ApiClient.Result() {
                        private void done() {
                            ready.delete(); stagedUpload=null;
                            uploading=false; upload.setEnabled(true); refresh.setEnabled(true);
                            images.setEnabled(fileType!=1); audio.setEnabled(fileType!=2); capture.setEnabled(fileType==1);
                        }
                        @Override public void success(JSONObject response) {
                            done();
                            if("successful".equals(response.optString("status"))) { message("上传成功，请在设备端更新列表后下载"); fileType=type; load(); }
                            else status.setText(response.optString("message","上传失败"));
                        }
                        @Override public void failure(String text) { done(); status.setText(text); }
                    });
                });
            } catch(IOException | RuntimeException error) {
                if(staged!=null) staged.delete(); stagedUpload=null;
                runOnUiThread(() -> {
                    if(isDestroyed()) return;
                    uploading=false; upload.setEnabled(true); refresh.setEnabled(true);
                    images.setEnabled(fileType!=1); audio.setEnabled(fileType!=2); capture.setEnabled(fileType==1);
                    status.setText("无法读取所选文件，请确认文件非空、不超过 32 MiB，且有足够缓存空间");
                });
            }
        });
    }
    private void remove(JSONObject file) {
        Map<String,String> p=deviceParameters(); p.put("file_id",file.optString("id")); p.put("tok",settings.token());
        api.postBody("/acho/api/file/delete/",form(p),true,new ApiClient.Result() {
            @Override public void success(JSONObject response) {
                if("successful".equals(response.optString("status"))) load();
                else status.setText(response.optString("message","删除失败"));
            }
            @Override public void failure(String text) { status.setText(text); }
        });
    }
    private static RequestBody form(Map<String,String> values) {
        okhttp3.FormBody.Builder builder=new okhttp3.FormBody.Builder();
        for(Map.Entry<String,String> entry:values.entrySet()) builder.add(entry.getKey(),entry.getValue());
        return builder.build();
    }
    private void download(Uri destination,JSONObject file) {
        if(downloadCall!=null) { message("已有下载正在进行"); return; }
        Map<String,String> p=deviceParameters(); p.put("file_id",file.optString("id"));
        Call call=ApiClient.HTTP.newCall(new Request.Builder().url(api.url("/acho/api/file/",p,true)).build());
        downloadCall=call; status.setText("正在保存文件…");
        downloads.execute(() -> {
            String result="保存成功"; boolean complete=false;
            try(Response response=call.execute()) {
                String mime=response.header("Content-Type","");
                if(response.body()==null || !response.isSuccessful() || !mime.startsWith("application/octet-stream"))
                    throw new IOException("服务器拒绝下载，请刷新列表或重新登录");
                try(InputStream input=response.body().byteStream(); OutputStream output=getContentResolver().openOutputStream(destination,"wt")) {
                    if(output==null) throw new IOException("无法写入所选位置");
                    byte[] buffer=new byte[8192]; int n; long count=0;
                    while((n=input.read(buffer))!=-1) {
                        count+=n; if(count>MAX_FILE) throw new IOException("文件超过 32 MiB"); output.write(buffer,0,n);
                    }
                    if(response.body().contentLength()>=0 && response.body().contentLength()!=count) throw new IOException("文件未完整下载");
                }
                complete=true;
            } catch(IOException | RuntimeException error) { result="下载失败，请检查登录状态、网络或保存位置"; }
            finally {
                if(!complete) try { DocumentsContract.deleteDocument(getContentResolver(),destination); } catch(Exception ignored) {}
                downloadCall=null;
            }
            String text=result;
            runOnUiThread(() -> { if(!isDestroyed()) status.setText(text); });
        });
    }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putInt("type",fileType); state.putInt("upload_type",pendingUploadType);
        if(cameraPhoto!=null) state.putString("camera_photo",cameraPhoto.getAbsolutePath());
        if(pendingDownload!=null) state.putString("download",pendingDownload.toString());
        super.onSaveInstanceState(state);
    }
    @Override protected void onDestroy() {
        if(downloadCall!=null) downloadCall.cancel();
        uploads.shutdownNow(); downloads.shutdownNow();
        if(stagedUpload!=null) stagedUpload.delete();
        if(isFinishing() && cameraPhoto!=null) cameraPhoto.delete();
        super.onDestroy();
    }
}
