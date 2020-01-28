package com.physis.pesticide.mixer.env;

import android.annotation.SuppressLint;

public class SystemEnv {

    public static final String BROKER_IP = "13.124.176.173";
    public static final String BROKER_PORT = "1883";

    public static final String DEVICE_ID = "TEMP10";

    public static final String TOPIC_SETUP_REQ = "PM/SETUP/REQ";
    public static final String TOPIC_SETUP_UPDATE = "PM/SETUP/UPDATE";
    public static final String TOPIC_SETUP_RES = "PM/SETUP/" + DEVICE_ID;

    public static final String TOPIC_STATE = "PM/STATE/" + DEVICE_ID;
    public static final String TOPIC_CONTROL = "PM/CONTROL/" + DEVICE_ID;

    @SuppressLint("DefaultLocale")
    public static String convertTimeUnit(String time){
        int timeSec = Integer.valueOf(time);
        int minute = timeSec / 60;
        int sec = timeSec % 60;
        return String.format("%02d", minute) + ":" + String.format("%02d", sec);
    }
}
