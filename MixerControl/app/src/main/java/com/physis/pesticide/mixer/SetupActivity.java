package com.physis.pesticide.mixer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.NumberPicker;
import android.widget.TimePicker;
import android.widget.Toast;

import com.physis.pesticide.mixer.env.SetupData;
import com.physis.pesticide.mixer.env.SystemEnv;
import com.physis.pesticide.mixer.mqtt.MQTTClient;

import java.util.Objects;

public class SetupActivity extends AppCompatActivity {

    private static final String TAG = SetupActivity.class.getSimpleName();

    private EditText etPowderMotorProceed, etPesticideMotorProceed;
    private NumberPicker npPowderSprayMinute, npPowderSpraySec;
    private NumberPicker npPesticideSprayMinute, npPesticideSpraySec;
    private NumberPicker npAgitatorMinute, npAgitatorSec;
    private NumberPicker npAirPumpMinute, npAirPumpSec;
    private Button btnSetup;

    private MQTTClient mqttClient;
    private SetupData setupData;

    private String options;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_setup);

        init();
    }

    @Override
    protected void onStart() {
        super.onStart();
        mqttClient.setHandler(handler);
        showSetupData();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @SuppressLint("HandlerLeak")
    private Handler handler = new Handler(){
        @Override
        public void handleMessage(@NonNull Message msg) {
            if (msg.what == MQTTClient.SUB_LISTEN) {
                Bundle data = (Bundle) msg.obj;
                String topic = data.getString("TOPIC");
                String message = data.getString("MESSAGE");
                Log.e(TAG, topic + " >> " + message);
                assert topic != null;
                if (topic.equals(SystemEnv.TOPIC_SETUP_RES)) {
                    assert message != null;
                    if(message.equals(options)){
                        setupData.setSetupData(message);
                        Toast.makeText(getApplicationContext(), "설정 정보를 변경하였습니다.", Toast.LENGTH_SHORT).show();
                        setResult(RESULT_OK, getIntent());
                        finish();
                    }else if(message.equals("")){
                        Toast.makeText(getApplicationContext(), "설정 변경에 실패하였습니다. 잠시 후 다시 시도해주세요.", Toast.LENGTH_SHORT).show();
                    }
                }
            }
        }
    };

    private View.OnClickListener clickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if(v.getId() == R.id.btn_setup){
                updateSetupData();
            }
        }
    };

    private void updateSetupData(){
        String powderMotorProceed = etPowderMotorProceed.getText().toString();
        String pesticideMotorProceed = etPesticideMotorProceed.getText().toString();

        if(powderMotorProceed.length() == 0 || pesticideMotorProceed.length() == 0)
            return;

        String powderWaterSpray = String.valueOf(npPowderSprayMinute.getValue() * 60 + npPowderSpraySec.getValue());
        String pesticideWaterSpray = String.valueOf(npPesticideSprayMinute.getValue() * 60 + npPesticideSpraySec.getValue());

        String agitatorTime = String.valueOf(npAgitatorMinute.getValue() * 60 + npAgitatorSec.getValue());
        String airPumpTime = String.valueOf(npAirPumpMinute.getValue() * 60 + npAirPumpSec.getValue());

        options = powderMotorProceed + "," + powderWaterSpray + "," + pesticideMotorProceed + ","
                + pesticideWaterSpray + "," + airPumpTime + "," + agitatorTime + "," + SystemEnv.DEVICE_ID;
        mqttClient.publish(SystemEnv.TOPIC_SETUP_UPDATE, options);
    }

    private void showSetupData(){
        etPowderMotorProceed.setText(setupData.getPowderMotorProceed());
        etPesticideMotorProceed.setText(setupData.getPesticideMotorProceed());
        int powderTime = Integer.valueOf(setupData.getPowderWaterSpray());
        npPowderSprayMinute.setValue(powderTime / 60);
        npPowderSpraySec.setValue(powderTime % 60);
        int pesticideTime = Integer.valueOf(setupData.getPesticideWaterSpray());
        npPesticideSprayMinute.setValue(pesticideTime / 60);
        npPesticideSpraySec.setValue(pesticideTime % 60);
        int airPumpTime = Integer.valueOf(setupData.getAirPumpTime());
        npAirPumpMinute.setValue(airPumpTime / 60);
        npAirPumpSec.setValue(airPumpTime % 60);
        int agitatorTime = Integer.valueOf(setupData.getAgitatorTime());
        npAgitatorMinute.setValue(agitatorTime / 60);
        npAgitatorSec.setValue(agitatorTime % 60);
    }

    private void init() {
        ActionBar actionBar = getSupportActionBar();
        Objects.requireNonNull(actionBar).setHomeAsUpIndicator(R.drawable.ic_back);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle("Setting");

        mqttClient = MQTTClient.getInstance();
        setupData = SetupData.getInstance();

        npPowderSprayMinute = findViewById(R.id.np_powder_spray_minute);
        npPowderSprayMinute.setMaxValue(0);
        npPowderSprayMinute.setMaxValue(59);
        npPowderSpraySec = findViewById(R.id.np_powder_spray_sec);
        npPowderSpraySec.setMaxValue(0);
        npPowderSpraySec.setMaxValue(59);
        npPesticideSprayMinute = findViewById(R.id.np_pesticide_spray_minute);
        npPesticideSprayMinute.setMaxValue(0);
        npPesticideSprayMinute.setMaxValue(59);
        npPesticideSpraySec = findViewById(R.id.np_pesticide_spray_sec);
        npPesticideSpraySec.setMaxValue(0);
        npPesticideSpraySec.setMaxValue(59);

        npAgitatorMinute = findViewById(R.id.np_agitator_minute);
        npAgitatorMinute.setMaxValue(0);
        npAgitatorMinute.setMaxValue(59);
        npAgitatorSec = findViewById(R.id.np_agitator_sec);
        npAgitatorSec.setMaxValue(0);
        npAgitatorSec.setMaxValue(59);

        npAirPumpMinute = findViewById(R.id.np_air_pump_minute);
        npAirPumpMinute.setMaxValue(0);
        npAirPumpMinute.setMaxValue(59);
        npAirPumpSec = findViewById(R.id.np_air_pump_sec);
        npAirPumpSec.setMaxValue(0);
        npAirPumpSec.setMaxValue(59);

        etPowderMotorProceed = findViewById(R.id.et_powder_motor_proceed);
        etPesticideMotorProceed = findViewById(R.id.et_pesticide_motor_proceed);

        btnSetup = findViewById(R.id.btn_setup);
        btnSetup.setOnClickListener(clickListener);
    }
}
