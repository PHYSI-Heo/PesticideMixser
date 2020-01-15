package com.physis.pesticide.mixer.custom;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.physis.pesticide.mixer.R;
import com.physis.pesticide.mixer.env.SystemEnv;

public class MixingStateView extends RelativeLayout {

    private static final String TAG = MixingStateView.class.getSimpleName();

    private RelativeLayout itemView;
    private TextView tvTitle, tvMotorProceed, tvSprayWater;
    private TextView tvMotorProceedState, tvSprayWaterState, tvMotorReturnState;
    private ImageView ivIcon;

    private String proceedHeight, sprayTime;
    private boolean enable = true;
    private int bgColor, blinkColor;

    private ObjectAnimator anim;
    private TextView blinkObj;
    private char mixState;

    public MixingStateView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
        setTypeArray(getContext().obtainStyledAttributes(attrs, R.styleable.MixingStateView));
    }

    private void initView() {
        String infService = Context.LAYOUT_INFLATER_SERVICE;
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(infService);
        assert inflater != null;
        View v = inflater.inflate(R.layout.view_mixing_state, this, false);
        addView(v);

        itemView = findViewById(R.id.rl_mixing_view);
        tvTitle = findViewById(R.id.tv_mixing_name);
        tvMotorProceed = findViewById(R.id.tv_motor_rotation);
        tvSprayWater = findViewById(R.id.tv_water_spray);
        tvMotorProceedState = findViewById(R.id.tv_motor_rotation_state);
        tvSprayWaterState = findViewById(R.id.tv_spray_water_state);
        tvMotorReturnState = findViewById(R.id.tv_motor_return_state);

        ivIcon = findViewById(R.id.iv_mixing_icon);
    }

    private void setTypeArray(TypedArray typedArray) {
        bgColor = typedArray.getResourceId(R.styleable.MixingStateView_backgroundColor, R.color.colorAccent);
        blinkColor = typedArray.getResourceId(R.styleable.MixingStateView_blinkColor, R.color.colorBlink);
        itemView.setBackgroundResource(bgColor);
        String title = typedArray.getString(R.styleable.MixingStateView_title);
        tvTitle.setText(title);
        int icon = typedArray.getResourceId(R.styleable.MixingStateView_titleImage, R.drawable.ic_water);
        ivIcon.setImageResource(icon);
        typedArray.recycle();
    }

    public void swapEnable(){
        this.enable = !enable;
        if(enable){
            itemView.setBackgroundResource(bgColor);
        }else{
            itemView.setBackgroundResource(R.color.colorDisable);
        }
    }

    public boolean getEnable(){
        return enable;
    }

    @SuppressLint("SetTextI18n")
    public void setMixingData(String proceedHeight, String sprayTime){
        this.proceedHeight = proceedHeight;
        this.sprayTime = sprayTime;
        tvMotorProceed.setText(this.proceedHeight + " Cm");
        tvSprayWater.setText(SystemEnv.convertTimeUnit(sprayTime));
    }

    @SuppressLint("WrongConstant")
    public void showBlink(char stage){
        if(stage == mixState)
            return;
        mixState = stage;
        switch (stage){
            case '1':
                startStageBlink(tvMotorProceedState);
                break;
            case '2':
                startStageBlink(tvSprayWaterState);
                break;
            case '3':
                startStageBlink(tvMotorReturnState);
                break;
            default:
                stopStageBlink();
        }
    }

    @SuppressLint("WrongConstant")
    private void startStageBlink(TextView obj){
        stopStageBlink();
        blinkObj = obj;
        anim = ObjectAnimator.ofInt(blinkObj, "backgroundColor", Color.WHITE, blinkColor, Color.WHITE);
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
            if(blinkObj != null)
                blinkObj.setBackgroundColor(Color.WHITE);
        }
    }

}
