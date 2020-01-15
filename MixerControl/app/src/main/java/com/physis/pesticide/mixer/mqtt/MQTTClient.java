package com.physis.pesticide.mixer.mqtt;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttMessageListener;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

public class MQTTClient {

    private static final String TAG = MQTTClient.class.getSimpleName();

    public static final int CONNECTED = 300;
    public static final int SUB_LISTEN = 301;
    public static final int DISCONNECTED = 302;

    private static MQTTClient mqttClient = null;
    private MqttClient mClient = null;
    private Handler handler = null;
    private MQTTConnectThread mConnectThread = null;

    private MQTTClient(){

    }

    public synchronized static MQTTClient getInstance(){
        if(mqttClient == null)
            mqttClient = new MQTTClient();
        return mqttClient;
    }

    public void setHandler(Handler handler){
        this.handler = handler;
    }

    public boolean isConnected(){
        if(mqttClient == null)
            return false;
        return mqttClient.isConnected();
    }

    public void disconnect(){
        if(mClient != null && mClient.isConnected()){
            try {
                mClient.disconnect();
                handler.obtainMessage(DISCONNECTED).sendToTarget();
                mClient = null;
            } catch (MqttException e) {
                e.printStackTrace();
            }
        }
        mConnectThread = null;
    }

    public void connect(Context context, String ip, String port){
        if(mClient != null && mClient.isConnected())
            return;
//         Get Android Device ID
        @SuppressLint("HardwareIds")
        String android_ID = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
        try {
            mClient = new MqttClient("tcp://" + ip + ":" + port, android_ID, new MemoryPersistence());
            mClient.setCallback(mqttCallback);
            mConnectThread = new MQTTConnectThread();
            mConnectThread.start();
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    public void subscribe(String topic){
        try {
            if(mClient == null)
                return;
            mClient.subscribeWithResponse(topic, 0, iMqttMessageListener);
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    private final IMqttMessageListener iMqttMessageListener = new IMqttMessageListener() {
        @Override
        public void messageArrived(String topic, MqttMessage message) throws Exception {
            Bundle bundle = new Bundle();
            bundle.putString("TOPIC", topic);
            bundle.putString("MESSAGE", new String(message.getPayload()));
            Message msg = new Message();
            handler.obtainMessage(SUB_LISTEN, bundle).sendToTarget();
        }
    };

    public void unsubscribe(String topic){
        try {
            if(mClient == null)
                return;
            mClient.unsubscribe(topic);
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    public void publish(String topic, String message){
        try {
            if(mClient == null)
                return;
            mClient.publish(topic, new MqttMessage(message.getBytes()));
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    private MqttCallback mqttCallback = new MqttCallback() {
        @Override
        public void connectionLost(Throwable cause) {
            Log.e(TAG, "# MQTT Connection Lost.");
            handler.obtainMessage(DISCONNECTED).sendToTarget();
        }

        @Override
        public void messageArrived(String topic, MqttMessage message) throws Exception {
//            Log.e(TAG, "messageArrived");
        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken token) {
            Log.e(TAG, "deliveryComplete");

        }
    };

    private class MQTTConnectThread extends Thread {

        @Override
        public void run() {
            super.run();
            try {
                if(mClient != null && mClient.isConnected())
                    return;
                MqttConnectOptions options = new MqttConnectOptions();
                options.setConnectionTimeout(2);
                mClient.connect(options);
            } catch (MqttException e ) {
                e.printStackTrace();
            }finally {
                assert mClient != null;
                handler.obtainMessage(CONNECTED, mClient.isConnected()).sendToTarget();
            }
        }
    }

}
