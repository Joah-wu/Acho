package com.example.websocktest;

import android.os.Bundle;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.example.websocktest.core.ApiClient;
import com.example.websocktest.core.AppSettings;

public abstract class BaseActivity extends AppCompatActivity {
    protected AppSettings settings;
    protected ApiClient api;
    @Override protected void onCreate(Bundle saved) {
        super.onCreate(saved); settings=new AppSettings(this); api=new ApiClient(settings);
    }
    protected void message(String text) { Toast.makeText(this,text,Toast.LENGTH_LONG).show(); }
    @Override protected void onDestroy() { api.close(); super.onDestroy(); }
}
