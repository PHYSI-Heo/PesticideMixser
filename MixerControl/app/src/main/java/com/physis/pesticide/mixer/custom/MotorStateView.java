package com.physis.pesticide.mixer.custom;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.physis.pesticide.mixer.R;

public class MotorStateView extends LinearLayout {

    private static final String TAG = MotorStateView.class.getSimpleName();

    private LinearLayout itemView;
    private TextView tvTitle, tvMotorTime;
    private ImageView ivIcon;

    private int bgColor, blinkColor;

    private ObjectAnimator anim;

    public MotorStateView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
        setTypeArray(getContext().obtainStyledAttributes(attrs, R.styleable.MotorStateView));
    }

    private void initView() {
        String infService = Context.LAYOUT_INFLATER_SERVICE;
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(infService);
        assert inflater != null;
        View v = inflater.inflate(R.layout.view_motor_state, this, false);
        addView(v);

        itemView = findViewById(R.id.ll_motor_view);
        tvTitle = findViewById(R.id.tv_motor_title);
        tvMotorTime = findViewById(R.id.tv_motor_time);
        ivIcon = findViewById(R.id.iv_motor_icon);
    }

    private void setTypeArray(TypedArray typedArray) {
        bgColor = typedArray.getResourceId(R.styleable.MotorStateView_motorBackColor, R.color.colorAccent);
        blinkColor = typedArray.getResourceId(R.styleable.MotorStateView_motorBlinkColor, R.color.colorBlink);
        itemView.setBackgroundResource(bgColor);
        String title = typedArray.getString(R.styleable.MotorStateView_motorTitle);
        tvTitle.setText(title);
        int icon = typedArray.getResourceId(R.styleable.MotorStateView_motorTitleImage, R.drawable.ic_water);
        ivIcon.setImageResource(icon);
        typedArray.recycle();
    }

    public void setTime(String time){
        tvMotorTime.setText(time);
    }

    @SuppressLint("WrongConstant")
    public void showBlink(char stage){
        if (stage == '1') {
            startStageBlink();
        } else {
            stopStageBlink();
        }
    }

    @SuppressLint("WrongConstant")
    private void startStageBlink(){
        stopStageBlink();
        anim = ObjectAnimator.ofInt(itemView, "backgroundColor", bgColor, blinkColor, bgColor);
        anim.setDuration(750);
        anim.setEvaluator(new ArgbEvaluator());
        anim.setRepeatCount(Animation.INFINITE);
        anim.setRepeatMode(Animation.RESTART);
        anim.start();
    }

    private void stopStageBlink(){
        if(anim != null) {
            anim.cancel();
            anim = null;
            itemView.setBackgroundResource(bgColor);
        }
    }

}
