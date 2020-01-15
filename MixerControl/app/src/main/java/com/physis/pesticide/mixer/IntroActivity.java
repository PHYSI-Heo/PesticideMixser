package com.physis.pesticide.mixer;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

import com.physis.pesticide.mixer.mqtt.MQTTClient;

public class IntroActivity extends AppCompatActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_intro);

        nextActivity();
    }

    @Override
    public void onBackPressed() {
//        super.onBackPressed();
    }

    private void nextActivity() {
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                startActivity(new Intent(IntroActivity.this, MainActivity.class));
                finish();
            }
        }, 1500);
    }

}
