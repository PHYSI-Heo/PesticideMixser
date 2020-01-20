#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>

#define swRX  13
#define swTX  15

const String AP_SSID      = "U+Net4C63";
const String AP_PASSWORD  = "6000214821";
const String MQTT_IP      = "13.124.176.173";
const int MQTT_PORT       = 1883;
const String DEVICE_ID = "TEMP20";
// Pub
const String TOPIC_SETUP_REQ = "PM/SETUP/REQ";
const String TOPIC_SEND_STATE = "PM/STATE/TEMP20";
// Sub
const String TOPIC_SETUP_RES = "PM/SETUP/TEMP20";
const String TOPIC_RECV_CON = "PM/CONTROL/TEMP20";

// Receive Header
String AP_CONN_STATE = "AC";
String REQ_SETUP_INFO = "RSI";
String PUB_DEVICE_STATE = "PD";
String ERR_RECV_MSG = "ERM";

// Send Header
String SEND_CON_MSG = "DC";
String SEND_SETUP_INFO = "SI";
String SEND_WIFI_DISCONNECT = "DW";
String SEND_CONN_MQTT = "CM";
String SEND_RE_REQ = "RQ";
String SEND_REFLASH = "RF";

SoftwareSerial swSerial(swRX, swTX);
WiFiClient wifiClient;
PubSubClient mqttClient;

extern "C" {
#include "user_interface.h"
}

String chipId;
String recvData = "";
String pushData = "";

void setup() {
  Serial.begin(115200);
  swSerial.begin(115200);
  Serial.println("! Start ESP.");

  wifi_set_sleep_type(NONE_SLEEP_T);

  chipId = String(ESP.getChipId());
  Serial.print(F("# ESP ID : "));
  Serial.println(chipId);

}

void loop() {
  if (mqttClient.loop()) {                             // ! MQTT CONNECTED.
    while (swSerial.available()) {                         // Read Serial Data
      char readByte = (char)swSerial.read();
      if (readByte != 0x00)  recvData += readByte;
      delay(1);
    }
    if (recvData != "") {
      if (confirmData(recvData)) {
        messageHandler();
      } else {
        writeData(SEND_RE_REQ);
      }
      recvData = "";
    }
  } else {                                            // ! MQTT DISCONNECTED
    if (WiFi.status() == WL_CONNECTED) {                  // WiFi Connected
      mqttConnect();
      // (to Mega) Connected MQTT
      writeData(SEND_CONN_MQTT);
    } else {                                                // WiFi Disconnected
      // (to Mega) Disconnected WiFi
      writeData(SEND_WIFI_DISCONNECT);
      connectWiFi();                                   // Start WiFi Connect
    }
  }
}


void messageHandler() {
  if (recvData.equals(REQ_SETUP_INFO)) {
    publisher(TOPIC_SETUP_REQ, DEVICE_ID);
  } else if (recvData.equals(AP_CONN_STATE)) {
    if (WiFi.status() != WL_CONNECTED) {
      writeData(SEND_WIFI_DISCONNECT);
    } else if (mqttClient.connected()) {
      writeData(SEND_CONN_MQTT);
    }
  } else if (recvData.equals(ERR_RECV_MSG)) {
    writeData(pushData);
  } else if (recvData.startsWith(PUB_DEVICE_STATE)) {
    publisher(TOPIC_SEND_STATE, recvData);
  }
}

/*  ===================================================
                        Check Sum
  =================================================== */
void writeData(String data) {
  pushData = data;
  int checkSum = 0;
  for (int i = 0; i < data.length(); i++) {
    checkSum += data[i];
  }
  String msg = "$" + String(checkSum) + "," + data + "#";
  delay(10);
  swSerial.print(msg);
}

bool confirmData(String data) {
  int sepIndex = data.indexOf(",");
  if (!data.startsWith("$") || !data.endsWith("#") || sepIndex == -1)
    return false;

  int checkSum = 0;
  recvData = data.substring(sepIndex + 1, data.length() - 1);
  for (int i = 0; i < recvData.length(); i++) {
    checkSum += recvData[i];
  }
  return data.substring(1, sepIndex).toInt() == checkSum;
}



/**   ===================================================
                        MQTT
  =================================================== */
void mqttConnect() {
  mqttClient.setServer(MQTT_IP.c_str(), MQTT_PORT);
  mqttClient.setCallback(subscribeCallbak);
  mqttClient.setClient(wifiClient);

  Serial.print(F("# MQTT Client ID : "));
  Serial.println(chipId);

  bool isConnected = mqttClient.connect(chipId.c_str());
  Serial.print(F("# MQTT Connect..."));
  if (isConnected)   {
    mqttClient.subscribe(TOPIC_SETUP_RES.c_str());
    mqttClient.subscribe(TOPIC_RECV_CON.c_str());
  }
}

void publisher(String topic, String data) {
  if (mqttClient.connected()) {
    bool result = (mqttClient.publish(topic.c_str(), data.c_str()) == 1);
    Serial.print(F("# Publish : "));
    Serial.print(topic);
    Serial.print(F(", "));
    Serial.print(data);
    Serial.print(F(" ( "));
    Serial.print(result);
    Serial.println(F(" )"));
  }
}

void subscribeCallbak(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  String subData = "";
  while (i < length)  subData += (char)payload[i++];
  Serial.print(F("# Subscribe Listen : "));
  Serial.println(subData);
  String subTopic = String(topic);
  if (subTopic.equals(TOPIC_SETUP_RES)) {
    writeData(SEND_SETUP_INFO + subData);
  } else if (subTopic.equals(TOPIC_RECV_CON)) {
    writeData(SEND_CON_MSG + subData);
    delay(100);
    writeData(SEND_REFLASH);
  }
}




/**   ===================================================
                        WiFi
  =================================================== */

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
    delay(5);
  }

  Serial.print(F("# WiFi Connecting."));
  WiFi.begin(AP_SSID.c_str(), AP_PASSWORD.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(1000);
  }
  Serial.println(F("."));
}
