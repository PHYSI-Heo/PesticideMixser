/*
  - 수화제 / 농약 모터는 따로 구동
  - 워터 펌프는 같이 구동
  - 에어 펌프는 수화제 제어에서만 활성화
*/
#include <SoftwareSerial.h>
#include <Stepper.h>

#define SW_RX  50
#define SW_TX  51

#define START_BUTTON              20
#define STOP_BUTTON               21

#define AGITATOR_MOTOR            22
#define AIR_PUMP                  23
#define PUMP_MOTOR                24
#define POWDER_VALVE              25
#define PESTICIDE_VALVE           26
#define STATUS_LED                27

#define PW_MOTOR_DIR              36
#define PW_MOTOR_CLK              37
#define PW_MOTOR_STEP             38

#define PT_MOTOR_DIR              31
#define PT_MOTOR_CLK              32
#define PT_MOTOR_STEP             33

#define INPUT_PULLDOWN  (0x3)

// Send Header
String REQ_SETUP_INFO       = "RSI";
String PUB_DEVICE_STATE     = "PD";
String ERR_RECV_MSG         = "ERM";
String AP_CONN_STATE        = "AC";

// Receive Header
String RECV_CON_MSG         = "DC";
String RECV_SETUP_INFO      = "SI";
String RECV_WIFI_DISCONNECT = "DW";
String RECV_CONN_MQTT       = "CM";
String RECV_RE_REQ          = "RQ";

// Step Motor
int STEP_REVOLUTION = 200;
int STEP_SPEED = 500;
int ROTATION_CNT = 5;

SoftwareSerial swSerial(SW_RX, SW_TX);
Stepper powderMotor(STEP_REVOLUTION, PW_MOTOR_CLK, PW_MOTOR_STEP);
Stepper pesticideMotor(STEP_REVOLUTION, PT_MOTOR_CLK, PT_MOTOR_STEP);

struct Material {
  int motorMoving;
  int motorCnt;
  unsigned long sprayStartTime;
  unsigned long sprayTimeLimit;
  int stage;
  int action;
  int valvePin;
  int dirPin;
  int clkPin;
};
Material material[2];

// Time Schedule
long checkAPTime = 0;
long checkAPInterval = 3000;
long pushStateTime = 0;
long pushStateInterval = 1000;
long stateLEDTime = 0;
long stateLEDInterval = 1000;

unsigned long airPumpStartTime;
unsigned long airPumpTimeLimit = 5 * 1000;
unsigned long agitatorStartTime;
unsigned long agitatorTimeLimit = 5 * 1000 * 60;

// Action State
int airPumpAction = 0;
int agitatorAction = 0;

// ESP Read/Write
String recvData = "";
String pushData = "";
bool mqttConnected = false;

void setup() {
  Serial.begin(115200);

  swSerial.begin(115200);
  delay(1000);

  pinMode(AGITATOR_MOTOR, OUTPUT);
  pinMode(AIR_PUMP, OUTPUT);
  pinMode(PUMP_MOTOR, OUTPUT);
  pinMode(POWDER_VALVE, OUTPUT);
  pinMode(PESTICIDE_VALVE, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  pinMode(START_BUTTON, INPUT_PULLDOWN);
  pinMode(STOP_BUTTON, INPUT_PULLDOWN);

  pinMode(PW_MOTOR_DIR, OUTPUT);
  pinMode(PW_MOTOR_CLK, OUTPUT);
  pinMode(PW_MOTOR_STEP, OUTPUT);
  pinMode(PT_MOTOR_DIR, OUTPUT);
  pinMode(PT_MOTOR_CLK, OUTPUT);
  pinMode(PT_MOTOR_STEP, OUTPUT);

  powderMotor.setSpeed(STEP_SPEED);
  pesticideMotor.setSpeed(STEP_SPEED);

  digitalWrite(PW_MOTOR_DIR, LOW);
  digitalWrite(PW_MOTOR_CLK, HIGH);
  digitalWrite(PW_MOTOR_STEP, HIGH);

  digitalWrite(PT_MOTOR_DIR, LOW);
  digitalWrite(PT_MOTOR_CLK, HIGH);
  digitalWrite(PT_MOTOR_STEP, HIGH);

  // 다채널 릴레이에서 동작에 대한 HIGH/LOW가 반대
  digitalWrite(AGITATOR_MOTOR, HIGH);
  digitalWrite(AIR_PUMP, HIGH);
  digitalWrite(PUMP_MOTOR, HIGH);
  digitalWrite(POWDER_VALVE, HIGH);
  digitalWrite(PESTICIDE_VALVE, HIGH);
  digitalWrite(STATUS_LED, HIGH);

  // Material 0 Index = Powder
  material[0].motorMoving = 5;
  material[0].sprayTimeLimit = 3 * 1000;
  material[0].valvePin = POWDER_VALVE;
  material[0].dirPin = PW_MOTOR_DIR;
  material[0].clkPin = PW_MOTOR_CLK;

  // Material 0 Index = Pesticide
  material[1].motorMoving = 5;
  material[1].sprayTimeLimit = 3 * 1000;
  material[1].valvePin = PESTICIDE_VALVE;
  material[1].dirPin = PT_MOTOR_DIR;
  material[1].clkPin = PT_MOTOR_CLK;

  attachInterrupt(digitalPinToInterrupt(START_BUTTON), startMixer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(STOP_BUTTON), stopMixer, CHANGE);

  for (int i = 0; i < 3; i++) {
    Serial.print(F("Loading.."));
    Serial.println(i);
    delay(1000);
  }
}

void loop() {
  long currentSec = millis();

  if (!mqttConnected && currentSec - checkAPTime > checkAPInterval) {
    writeData(AP_CONN_STATE);
    checkAPTime = currentSec;
  }

  readMessage();

  agitatorController();
  MaterialController(0);
  MaterialController(1);
  airPumpController();

  if (mqttConnected && currentSec - pushStateTime > pushStateInterval) {
    pushStateMessage();
  }

  if (material[0].action == 0 && material[1].action == 0 && agitatorAction == 0) {
    digitalWrite(STATUS_LED, HIGH);
  }
  delay(50);
}



/*   ===================================================
                      Motor Controller
  =================================================== */

void rotationMotro(int index, bool isMoving) {
  if (index == 0) {
    digitalWrite(material[index].dirPin, !isMoving);
    powderMotor.step(STEP_REVOLUTION * 4 * ROTATION_CNT);
  } else {
    digitalWrite(material[index].dirPin, isMoving);
    pesticideMotor.step(STEP_REVOLUTION * 4 * ROTATION_CNT);
  }
}


// Stage Info.
// 0 = Stop, 1 = Moving Motor , 2 = Spray Water, 3 = Air Pump, 4 = Return Motor
void MaterialController(int index) {
  if (material[index].action) {
    if (material[index].stage == 1) {                                // Moving Motor
      while (material[index].motorCnt < material[index].motorMoving) {
        pushStateMessage();
        rotationMotro(index, true);
        material[index].motorCnt++;
        Serial.print(F("## [ Start Motor ] Index : "));
        Serial.print(index);
        Serial.print(F(" // Count.."));
        Serial.println(material[index].motorCnt);
      }
      material[index].stage == 2;
    } else if (material[index].stage == 2) {                        // Spray Water
      unsigned long mSec = millis();
      if (material[index].sprayStartTime == 0) {
        material[index].sprayStartTime = mSec;
        digitalWrite(material[index].valvePin , LOW);
        digitalWrite(PUMP_MOTOR, LOW);

        Serial.print(F("## [[ "));
        Serial.print(index);
        Serial.println(F(" ]] Start Water Spray.."));
      }
      if (mSec - material[index].sprayStartTime >= material[index].sprayTimeLimit) {
        digitalWrite(material[index].valvePin, HIGH);
        material[index].stage = 4;
        delay(10);

        Serial.print(F("> [[ "));
        Serial.print(index);
        Serial.println(F(" ]] Stop Water Spray.."));
        StopPumpMotor();
        if (index == 0) {
          material[index].stage = 3;
          airPumpAction = 1;
          airPumpController();
        }
      }
    } else if (material[index].stage == 4) {                          // Moving Return
      int checkIndex = index == 0 ? 1 : 0;
      if (material[checkIndex].action && material[checkIndex].stage != 4)
        return;
      returnMaterialMotor(index);
    }
  } else {
    digitalWrite(material[index].valvePin, HIGH);
    digitalWrite(material[index].clkPin, HIGH);
    returnMaterialMotor(index);
  }
}

void returnMaterialMotor(int index) {
  while (material[index].motorCnt > 0) {
    material[index].stage = 3;
    pushStateMessage();
    rotationMotro(index, false);
    material[index].motorCnt--;
    Serial.print(F("## [ Return Motor ] Index : "));
    Serial.print(index);
    Serial.print(F(" // Count.."));
    Serial.println(material[index].motorCnt);
  }
  material[index].stage = 0;
  material[index].action = 0;
}

void StopPumpMotor() {
  if (material[0].stage == 2 || material[1].stage == 2) {
    return;
  }
  digitalWrite(PUMP_MOTOR, HIGH);
  Serial.println(F("> [[ Water Pump Motor ]] Stop.."));
}

void airPumpController() {
  if (airPumpAction) {
    unsigned long mSec = millis();
    if (airPumpStartTime == 0) {
      airPumpStartTime = mSec;
      digitalWrite(AIR_PUMP, LOW);
      Serial.println(F("> [[ Air Pump ]] Start Motor.."));
    }
    if (mSec - airPumpStartTime >= airPumpTimeLimit) {
      airPumpAction = airPumpStartTime = 0;
      digitalWrite(AIR_PUMP, HIGH);
      Serial.println(F("> [[ Air Pump ]] Stop Motor.."));
      material[0].stage = 4;
    }
  } else {
    digitalWrite(AIR_PUMP, HIGH);
  }
}

void agitatorController() {
  if (agitatorAction) {
    unsigned long mSec = millis();
    if (agitatorStartTime == 0) {
      agitatorStartTime = mSec;
      digitalWrite(AGITATOR_MOTOR, LOW);
      Serial.println(F("> [[ Agitator ]] Start Motor.."));
    }
    if (mSec - agitatorStartTime >= agitatorTimeLimit) {
      agitatorAction = agitatorStartTime = 0;
      digitalWrite(AGITATOR_MOTOR, HIGH);
      Serial.println(F("> [[ Agitator ]] Stop Motor.."));
    }
  } else {
    digitalWrite(AGITATOR_MOTOR, HIGH);
  }
}

/*   ===================================================
                        Utils
  =================================================== */
void pushStateMessage() {
  String stateMsg = PUB_DEVICE_STATE + String(material[0].stage) + String(material[1].stage) + String(agitatorAction);
  delay(10);
  writeData(stateMsg);
  pushStateTime = millis();
  //  Serial.print(F("# Publish State Message : "));
  //  Serial.println(stateMsg);
}

void startMixer() {
  if (material[0].action == 0 && material[1].action == 0 && agitatorAction == 0) {
    Serial.println("# Start Mixing..");
    digitalWrite(STATUS_LED, LOW);
    material[0].action = material[1].action = agitatorAction = 1;
    material[0].stage = 1;
    material[0].sprayStartTime = 0;
    material[1].stage = 1;
    material[1].sprayStartTime = 0;
  }
}

void stopMixer() {
  Serial.println("# Stop Mixing..");
  digitalWrite(PUMP_MOTOR, HIGH);
  material[0].action = material[1].action = agitatorAction = airPumpAction = 0;
  agitatorStartTime = airPumpStartTime = 0;
}

/*   ===================================================
                      Message Handler
  =================================================== */
void readMessage() {
  while (swSerial.available()) {
    char readByte = (char)swSerial.read();
    if (readByte != 0x00)  recvData += readByte;
    delay(1);
  }
  if (recvData != "") {
    Serial.print(F("# Recv Msg : "));
    Serial.println(recvData);
    if (confirmData(recvData)) {
      messageHandler();
    } else {
      writeData(ERR_RECV_MSG);
    }
    recvData = "";
  }
  delay(10);
}

void messageHandler() {
  if (recvData.equals(RECV_WIFI_DISCONNECT)) {
    mqttConnected = false;
    Serial.println(F(">> Wifi Disconnected.."));
  } else if (recvData.equals(RECV_CONN_MQTT)) {
    mqttConnected = true;
    Serial.println(F(">> Mqtt Connected.."));
    writeData(REQ_SETUP_INFO);
  } else if (recvData.equals(RECV_RE_REQ)) {
    writeData(pushData);
  } else if (recvData.startsWith(RECV_SETUP_INFO)) {
    setSettingValues();
  } else if (recvData.startsWith(RECV_CON_MSG)) {
    setControlValue();
  }
}

void setControlValue() {
  material[0].action = String(recvData[2]).toInt();
  material[1].action = String(recvData[3]).toInt();
  agitatorAction = String(recvData[4]).toInt();

  if (material[0].action == 0 && material[1].action == 0 && agitatorAction == 0) {
    digitalWrite(PUMP_MOTOR, HIGH);
    airPumpAction = 0;
  } else {
    if (!digitalRead(STATUS_LED))
      return;
    digitalWrite(STATUS_LED, LOW);
  }

  if (material[0].action) {
    material[0].stage = 1;
    material[0].sprayStartTime = 0;
  }
  if (material[1].action) {
    material[1].stage = 1;
    material[1].sprayStartTime = 0;
  }

  Serial.print("# Action >> Powder : ");
  Serial.print(material[0].action);
  Serial.print(", Pesticide : ");
  Serial.print( material[1].action);
  Serial.print(", Agitator : ");
  Serial.println(agitatorAction);
  delay(5);
}


void setSettingValues() {
  recvData = recvData.substring(2, recvData.length());
  unsigned long infos[6];

  int position = 0;
  while (true)
  {
    int sepIndex = recvData.indexOf(",");
    if (sepIndex != -1)
    {
      infos[position++] = recvData.substring(0, sepIndex).toInt();
      recvData = recvData.substring(sepIndex + 1);
      delay(5);
    }
    else
    {
      infos[position] = recvData.toInt();
      break;
    }
  }

  material[0].motorMoving = infos[0];
  material[0].sprayTimeLimit = infos[1] * 1000;
  material[1].motorMoving = infos[2];
  material[1].sprayTimeLimit = infos[3] * 1000;
  airPumpTimeLimit = infos[4] * 1000;
  agitatorTimeLimit = infos[5] * 1000 * 60;

  Serial.print(F("# Setting Infos : "));
  Serial.print(material[0].motorMoving);
  Serial.print(", ");
  Serial.print(material[0].sprayTimeLimit);
  Serial.print(", ");
  Serial.print(material[1].motorMoving);
  Serial.print(", ");
  Serial.print(material[1].sprayTimeLimit);
  Serial.print(", ");
  Serial.print(airPumpTimeLimit);
  Serial.print(", ");
  Serial.println(agitatorTimeLimit);
  delay(5);
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
  delay(5);
  int checkSum = 0;
  recvData = data.substring(sepIndex + 1, data.length() - 1);
  for (int i = 0; i < recvData.length(); i++) {
    checkSum += recvData[i];
  }
  delay(5);
  return data.substring(1, sepIndex).toInt() == checkSum;
}
