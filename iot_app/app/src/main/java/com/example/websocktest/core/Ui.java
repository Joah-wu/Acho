package com.example.websocktest.core;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.InputType;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

public final class Ui {
    private Ui() {}
    public static int dp(Activity context,int n) { return Math.round(n*context.getResources().getDisplayMetrics().density); }
    public static LinearLayout screen(Activity activity,String title) {
        ScrollView scroll=new ScrollView(activity);
        scroll.setFillViewport(true); scroll.setBackgroundColor(Color.rgb(246,248,250));
        LinearLayout content=new LinearLayout(activity); content.setOrientation(LinearLayout.VERTICAL);
        int padding=dp(activity,20); content.setPadding(padding,padding,padding,padding);
        scroll.addView(content); activity.setContentView(scroll);
        WindowInsetsControllerCompat controller=new WindowInsetsControllerCompat(activity.getWindow(),scroll);
        controller.setAppearanceLightStatusBars(true); controller.setAppearanceLightNavigationBars(true);
        ViewCompat.setOnApplyWindowInsetsListener(scroll,(view,insets) -> {
            androidx.core.graphics.Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars()|WindowInsetsCompat.Type.ime());
            view.setPadding(bars.left,bars.top,bars.right,bars.bottom); return insets;
        });
        TextView heading=text(activity,content,title); heading.setTextSize(28); heading.setTypeface(null,Typeface.BOLD);
        return content;
    }
    public static TextView text(Activity a,LinearLayout parent,String value) {
        TextView view=new TextView(a); view.setText(value); view.setTextColor(Color.rgb(30,45,55));
        view.setTextSize(16); view.setPadding(0,dp(a,8),0,dp(a,8));
        parent.addView(view,new LinearLayout.LayoutParams(-1,-2)); return view;
    }
    public static EditText input(Activity a,LinearLayout parent,String hint,boolean secret) {
        EditText edit=new EditText(a); edit.setImportantForAutofill(View.IMPORTANT_FOR_AUTOFILL_NO); edit.setHint(hint); edit.setSingleLine(true); edit.setTextColor(Color.BLACK);
        edit.setInputType(InputType.TYPE_CLASS_TEXT|(secret?InputType.TYPE_TEXT_VARIATION_PASSWORD:InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
        parent.addView(edit,new LinearLayout.LayoutParams(-1,dp(a,56))); return edit;
    }
    public static Button button(Activity a,LinearLayout parent,String title,View.OnClickListener listener) {
        Button button=new Button(a); button.setText(title); button.setAllCaps(false); button.setMinHeight(dp(a,48));
        LinearLayout.LayoutParams params=new LinearLayout.LayoutParams(-1,-2); params.topMargin=dp(a,8);
        parent.addView(button,params); button.setOnClickListener(listener); return button;
    }
}
