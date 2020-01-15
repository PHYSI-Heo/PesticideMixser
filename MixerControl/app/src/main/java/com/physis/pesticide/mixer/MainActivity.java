package com.physis.pesticide.mixer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.animation.Animation;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.physis.pesticide.mixer.custom.MixingStateView;
import com.physis.pesticide.mixer.custom.MotorStateView;
import com.physis.pesticide.mixer.env.SetupData;
import com.physis.pesticide.mixer.env.SystemEnv;
import com.physis.pesticide.mixer.mqtt.MQTTClient;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int REQ_SETUP_CODE = 2020;

    private MixingStateView msvPowder, msvPesticide;
    private MotorStateView msvAirPump, msvAgitator;
    private Button btnStart, btnStop;

    private ObjectAnimator anim;

    private MQTTClient mqttClient;
    private SetupData setupData;

    private String mixerStateMsg;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init();
    }

    @Override
    protected void onStart() {
        super.onStart();
        mqttClient.setHandler(handler);
        mqttClient.connect(getApplicationContext(), SystemEnv.BROKER_IP, SystemEnv.BROKER_PORT);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mqttClient.disconnect();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.toolbar_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()){
            case R.id.action_setting:
                startActivityForResult(new Intent(MainActivity.this, SetupActivity.class), REQ_SETUP_CODE);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == REQ_SETUP_CODE && resultCode == RESULT_OK){
            msvPowder.setMixingData(setupData.getPowderMotorProceed(), setupData.getPowderWaterSpray());
            msvPesticide.setMixingData(setupData.getPesticideMotorProceed(), setupData.getPesticideWaterSpray());
            msvAirPump.setTime(SystemEnv.convertTimeUnit(setupData.getAirPumpTime()));
            msvAgitator.setTime(SystemEnv.convertTimeUnit(setupData.getAgitatorTime()));
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        @Override
        public void handleMessage(@NonNull Message msg) {
            switch (msg.what){
                case MQTTClient.CONNECTED:
                    boolean result = (boolean) msg.obj;
                    Toast.makeText(getApplicationContext(), "# MQTT CONNECTED : " + result, Toast.LENGTH_SHORT).show();
                    if(result){
                        mqttClient.subscribe(SystemEnv.TOPIC_SETUP_RES);
                        mqttClient.subscribe(SystemEnv.TOPIC_STATE);
                        mqttClient.publish(SystemEnv.TOPIC_SETUP_REQ, SystemEnv.DEVICE_ID);
                    }
                    break;
                case MQTTClient.DISCONNECTED:
                    Toast.makeText(getApplicationContext(), "# MQTT DISCONNECTED.", Toast.LENGTH_SHORT).show();
                    break;
                case MQTTClient.SUB_LISTEN:
                    Bundle data = (Bundle) msg.obj;
                    String topic = data.getString("TOPIC");
                    String message = data.getString("MESSAGE");
                    Log.e(TAG, topic + " >> " + message );
                    assert topic != null;
                    assert message != null;
                    if(topic.equals(SystemEnv.TOPIC_SETUP_RES)){
                        setupData.setSetupData(message);
                        msvPowder.setMixingData(setupData.getPowderMotorProceed(), setupData.getPowderWaterSpray());
                        msvPesticide.setMixingData(setupData.getPesticideMotorProceed(), setupData.getPesticideWaterSpray());
                        msvAirPump.setTime(SystemEnv.convertTimeUnit(setupData.getAirPumpTime()));
                        msvAgitator.setTime(SystemEnv.convertTimeUnit(setupData.getAgitatorTime()));
                    }else if(topic.equals(SystemEnv.TOPIC_STATE)){
                        message = message.substring(2);
                        boolean  controlEnable = message.equals("0000");
                        btnStart.setEnabled(controlEnable);
                        msvPowder.setEnabled(controlEnable);
                        msvPesticide.setEnabled(controlEnable);

                        msvPowder.showBlink(message.charAt(0));
                        msvPesticide.showBlink(message.charAt(1));
                        msvAirPump.showBlink(message.charAt(2));
                        msvAgitator.showBlink(message.charAt(3));
                    }
                    break;
            }
        }
    };

    private View.OnClickListener clickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            switch (v.getId()){
                case R.id.btn_mixing_start:
                    sendMixingControl(msvPowder.getEnable() ? "1" : "0", msvPesticide.getEnable() ? "1" : "0", "1", "1");
                    btnStart.setEnabled(false);
                    break;
                case R.id.btn_mixing_stop:
                    sendMixingControl("0", "0", "0", "0");
                    break;
                case R.id.msv_wettable_powder:
                    msvPowder.swapEnable();
                    break;
                case R.id.msv_pesticide:
                    msvPesticide.swapEnable();
                    break;
            }
        }
    };


    private void sendMixingControl(String powderState, String pesticideState, String airPumpState, String agitatorState){
        String controlMsg = powderState + pesticideState + airPumpState + agitatorState;
        mqttClient.publish(SystemEnv.TOPIC_CONTROL, controlMsg);
    }

    private void init(){
        mqttClient = MQTTClient.getInstance();
        setupData = SetupData.getInstance();

        msvPowder = findViewById(R.id.msv_wettable_powder);
        msvPowder.setOnClickListener(clickListener);
        msvPesticide = findViewById(R.id.msv_pesticide);
        msvPesticide.setOnClickListener(clickListener);

        btnStart = findViewById(R.id.btn_mixing_start);
        btnStart.setOnClickListener(clickListener);
        btnStop = findViewById(R.id.btn_mixing_stop);
        btnStop.setOnClickListener(clickListener);

        msvAirPump = findViewById(R.id.msv_air_pump);
        msvAgitator = findViewById(R.id.msv_agitator);

        btnStart.setEnabled(false);
        msvPowder.setEnabled(false);
        msvPesticide.setEnabled(false);
    }
}
