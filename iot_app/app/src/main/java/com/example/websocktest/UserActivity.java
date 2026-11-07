package com.example.websocktest;

import android.os.Bundle;
import android.text.InputType;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.example.websocktest.core.ApiClient;
import com.example.websocktest.core.Ui;
import org.json.JSONObject;
import java.util.HashMap;
import java.util.Map;

public class UserActivity extends BaseActivity {
    private EditText username,password,confirmation;
    private Button login,register;
    private TextView status;
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout root=Ui.screen(this,"账号登录");
        Ui.text(this,root,"服务器："+settings.server());
        username=Ui.input(this,root,"用户名",false); username.setText(settings.username());
        username.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        password=Ui.input(this,root,"密码",true);
        confirmation=Ui.input(this,root,"注册时请再次输入密码",true);
        username.setImportantForAutofill(android.view.View.IMPORTANT_FOR_AUTOFILL_YES);
        password.setImportantForAutofill(android.view.View.IMPORTANT_FOR_AUTOFILL_YES);
        confirmation.setImportantForAutofill(android.view.View.IMPORTANT_FOR_AUTOFILL_YES);
        username.setAutofillHints(android.view.View.AUTOFILL_HINT_USERNAME);
        password.setAutofillHints(android.view.View.AUTOFILL_HINT_PASSWORD);
        confirmation.setAutofillHints("newPassword");
        login=Ui.button(this,root,"登录",v -> authenticate(false));
        register=Ui.button(this,root,"注册并登录",v -> authenticate(true));
        status=Ui.text(this,root,"注册密码至少 8 位。服务器地址在首页设置。");
        Ui.button(this,root,"返回",v -> finish());
    }
    private void authenticate(boolean create) {
        String name=username.getText().toString().trim(), secret=password.getText().toString();
        if(name.isEmpty() || secret.isEmpty()) { status.setText("请输入用户名和密码"); return; }
        if(create && (secret.length()<8 || !secret.equals(confirmation.getText().toString()))) {
            status.setText("注册密码至少 8 位，且两次输入必须一致"); return;
        }
        login.setEnabled(false); register.setEnabled(false); status.setText(create?"正在创建账号…":"正在登录…");
        Map<String,String> form=new HashMap<>(); form.put("username",name); form.put("password",secret);
        api.post(create?"/welcome/api/register/":"/welcome/api/login/",form,new ApiClient.Result() {
            @Override public void success(JSONObject data) {
                login.setEnabled(true); register.setEnabled(true);
                if("successful".equals(data.optString("status")) && !data.optString("token").isEmpty()) {
                    settings.login(name,data.optString("token"));
                    setResult(RESULT_OK); finish();
                } else status.setText(data.optString("message","登录失败，请检查用户名和密码"));
            }
            @Override public void failure(String text) { login.setEnabled(true); register.setEnabled(true); status.setText(text); }
        });
    }
}
