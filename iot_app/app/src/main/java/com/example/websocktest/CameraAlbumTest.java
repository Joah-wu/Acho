package com.example.websocktest;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import com.example.websocktest.core.IndexedImage;
import com.example.websocktest.core.Ui;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Keeps the original photo/album/dither/bin tools without a Windows-only NDK dependency. */
public class CameraAlbumTest extends BaseActivity {
    private ImageView preview;
    private TextView status;
    private File photo;
    private byte[] binary;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private final ActivityResultLauncher<String> album=registerForActivityResult(new ActivityResultContracts.GetContent(),uri -> { if(uri!=null) process(uri); });
    private final ActivityResultLauncher<Uri> camera=registerForActivityResult(new ActivityResultContracts.TakePicture(),ok -> {
        if(ok && photo!=null) process(Uri.fromFile(photo));
    });
    private final ActivityResultLauncher<String> cameraPermission=registerForActivityResult(new ActivityResultContracts.RequestPermission(),allowed -> {
        if(allowed) takePhoto(); else message("拍照需要相机权限，也可以从相册选择图片");
    });
    private final ActivityResultLauncher<String> export=registerForActivityResult(new ActivityResultContracts.CreateDocument("application/octet-stream"),uri -> {
        if(uri==null || binary==null) return;
        byte[] data=binary.clone();
        worker.execute(() -> {
            try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")) {
                if(out==null) throw new IOException(); out.write(data);
                showStatus("点阵文件已导出");
            } catch(IOException error) { showStatus("导出失败，请检查保存位置"); }
        });
    });
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        if(state!=null && state.containsKey("photo")) photo=new File(state.getString("photo"));
        LinearLayout root=Ui.screen(this,"本地图片工具");
        Ui.text(this,root,"拍照或从相册选图，按当前 128×128 屏幕生成黑白点阵；也可返回设备文件页上传原图，由服务器生成点阵。");
        preview=new ImageView(this); preview.setAdjustViewBounds(true); preview.setScaleType(ImageView.ScaleType.FIT_CENTER);
        root.addView(preview,new LinearLayout.LayoutParams(-1,Ui.dp(this,260)));
        status=Ui.text(this,root,"");
        Ui.button(this,root,"拍照",v -> {
            if(ContextCompat.checkSelfPermission(this,Manifest.permission.CAMERA)==PackageManager.PERMISSION_GRANTED) takePhoto();
            else cameraPermission.launch(Manifest.permission.CAMERA);
        });
        Ui.button(this,root,"从相册选择",v -> album.launch("image/*"));
        Ui.button(this,root,"导出 LVGL 点阵文件",v -> { if(binary==null) message("请先选择图片"); else export.launch("dithered_img.bin"); });
        Ui.button(this,root,"读取上次点阵",v -> worker.execute(() -> {
            try { display(IndexedImage.decode(Files.readAllBytes(new File(getFilesDir(),"dithered_img.bin").toPath()))); }
            catch(IOException | IllegalArgumentException error) { showStatus("还没有可读取的点阵文件"); }
        }));
        Ui.button(this,root,"返回",v -> finish());
    }
    private void takePhoto() {
        try {
            if(photo!=null) photo.delete();
            photo=File.createTempFile("acho-photo-",".jpg",getCacheDir());
            camera.launch(FileProvider.getUriForFile(this,getPackageName()+".fileprovider",photo));
        } catch(IOException | RuntimeException error) { message("无法打开相机，请检查相机应用与权限"); }
    }
    private void process(Uri uri) {
        status.setText("正在生成点阵…");
        worker.execute(() -> {
            try {
                BitmapFactory.Options bounds=new BitmapFactory.Options(); bounds.inJustDecodeBounds=true;
                try(InputStream input=getContentResolver().openInputStream(uri)) { BitmapFactory.decodeStream(input,null,bounds); }
                if(bounds.outWidth<1 || bounds.outHeight<1) throw new IOException();
                BitmapFactory.Options options=new BitmapFactory.Options(); options.inSampleSize=1;
                while(Math.max(bounds.outWidth,bounds.outHeight)/options.inSampleSize>1024) options.inSampleSize*=2;
                Bitmap original;
                try(InputStream input=getContentResolver().openInputStream(uri)) { original=BitmapFactory.decodeStream(input,null,options); }
                if(original==null) throw new IOException();
                try(InputStream metadata=getContentResolver().openInputStream(uri)) {
                    if(metadata!=null) {
                        int orientation=new ExifInterface(metadata).getAttributeInt(ExifInterface.TAG_ORIENTATION,ExifInterface.ORIENTATION_NORMAL);
                        Matrix transform=new Matrix();
                        switch(orientation) {
                            case ExifInterface.ORIENTATION_ROTATE_90: transform.setRotate(90); break;
                            case ExifInterface.ORIENTATION_ROTATE_180: transform.setRotate(180); break;
                            case ExifInterface.ORIENTATION_ROTATE_270: transform.setRotate(270); break;
                            case ExifInterface.ORIENTATION_FLIP_HORIZONTAL: transform.setScale(-1,1); break;
                            case ExifInterface.ORIENTATION_FLIP_VERTICAL: transform.setScale(1,-1); break;
                            case ExifInterface.ORIENTATION_TRANSPOSE: transform.setRotate(90); transform.postScale(-1,1); break;
                            case ExifInterface.ORIENTATION_TRANSVERSE: transform.setRotate(270); transform.postScale(-1,1); break;
                            default: break;
                        }
                        if(!transform.isIdentity()) {
                            Bitmap oriented=Bitmap.createBitmap(original,0,0,original.getWidth(),original.getHeight(),transform,true);
                            if(oriented!=original) { original.recycle(); original=oriented; }
                        }
                    }
                } catch(IOException ignored) { /* Providers may omit image metadata. */ }
                double scale=Math.min(1,Math.min(128.0/original.getWidth(),128.0/original.getHeight()));
                Bitmap resized=Bitmap.createScaledBitmap(original,Math.max(1,(int)(original.getWidth()*scale)),Math.max(1,(int)(original.getHeight()*scale)),true);
                int[] pixels=new int[resized.getWidth()*resized.getHeight()];
                resized.getPixels(pixels,0,resized.getWidth(),0,0,resized.getWidth(),resized.getHeight());
                IndexedImage image=IndexedImage.dither(pixels,resized.getWidth(),resized.getHeight());
                if(resized!=original) resized.recycle(); original.recycle();
                Files.write(new File(getFilesDir(),"dithered_img.bin").toPath(),image.binary);
                display(image);
            } catch(IOException | RuntimeException error) { showStatus("图片无法读取，请选择有效的图片文件"); }
        });
    }
    private void display(IndexedImage image) {
        Bitmap bitmap=Bitmap.createBitmap(image.pixels,image.width,image.height,Bitmap.Config.ARGB_8888);
        runOnUiThread(() -> {
            if(isDestroyed()) { bitmap.recycle(); return; }
            binary=image.binary; preview.setImageBitmap(bitmap); status.setText("已生成 "+image.width+"×"+image.height+" 点阵，"+binary.length+" 字节");
        });
    }
    private void showStatus(String text) { runOnUiThread(() -> { if(!isDestroyed()) status.setText(text); }); }
    @Override protected void onSaveInstanceState(Bundle state) {
        if(photo!=null) state.putString("photo",photo.getAbsolutePath()); super.onSaveInstanceState(state);
    }
    @Override protected void onDestroy() { worker.shutdownNow(); if(isFinishing() && photo!=null) photo.delete(); super.onDestroy(); }
}
