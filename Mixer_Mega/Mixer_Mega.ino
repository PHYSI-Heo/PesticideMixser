#include <SoftwareSerial.h>
#include <Stepper.h>

#define swRX  11
#define swTX  12

#define agitatorMotorPin          32
#define powderValvePin            33
#define pesticideValvePin         34

#define powderMotorStepPin        41
#define powderMotorPWMPin         42
#define powderMotorDIRPin         43

#define pesticideMotorStepPin     44
#define pesticideMotorPWMPin      45
#define pesticideMotorDIRPin      46

const int stepRevolution = 200;
const int stepSpeed = 500;
int rotationSize = 3;

SoftwareSerial swSerial(swRX, swTX);
Stepper powderMotor(stepRevolution, powderMotorStepPin, powderMotorPWMPin);
Stepper pesticideMotor(stepRevolution, pesticideMotorStepPin, pesticideMotorPWMPin);

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

// Setup Options
int powderMotorMoving = 15;
int pesticideMotorMoving = 15;
long powderSprayTime = 30 * 1000;
long pesticideSprayTime = 30 * 1000;
long agitatorTime = 120 * 1000;

// Time Schedule
long checkAPTime = 0;
long checkAPInterval = 3000;
long pushStateTime = 0;
long pushStateInterval = 500;
long agitatorStart = 0;
long powderSprayStart = 0;
long pesticideSprayStart = 0;

// Control Order
int powderControl = 0;
int pesticideControl = 0;
int agitatorControl = 0;

String recvData = "";
String pushData = "";
bool mqttPassable = false;

// - 0 : Stop, 1 : Motor Move, 2 : Spray Water, 3 : Motor Return
int powderStage = 0;
int pesticideStage = 0;
int powderMotorCnt = 0;
int pesticideMotorCnt = 0;
String stateMsg = "";

void setup() {
  Serial.begin(115200);
  swSerial.begin(115200);
  delay(1000);

  pinMode(agitatorMotorPin, OUTPUT);
  pinMode(powderValvePin, OUTPUT);
  pinMode(pesticideValvePin, OUTPUT);

  pinMode(powderMotorStepPin, OUTPUT);
  pinMode(powderMotorPWMPin, OUTPUT);
  pinMode(powderMotorDIRPin, OUTPUT);
  pinMode(pesticideMotorStepPin, OUTPUT);
  pinMode(pesticideMotorPWMPin, OUTPUT);
  pinMode(pesticideMotorDIRPin, OUTPUT);

  powderMotor.setSpeed(stepSpeed);
  pesticideMotor.setSpeed(stepSpeed);

  digitalWrite(powderMotorDIRPin, LOW);
  digitalWrite(pesticideMotorDIRPin, LOW);

  Serial.println("! Start Mega.");
  delay(1000);
}

void loop() {
  long currentSec = millis();
  readMessage();

  if (!mqttPassable && currentSec - checkAPTime > checkAPInterval) {
    writeData(AP_CONN_STATE);
    checkAPTime = currentSec;
  }

  powderController();
  pesticideController();
  agitatorController();

  if (mqttPassable && (pushStateTime == 0 || currentSec - pushStateTime > pushStateInterval)) {
    pushStateMessage();
    pushStateTime = currentSec;
  }

  delay(50);
}


void pushStateMessage() {
  String stateMsg = PUB_DEVICE_STATE + String(powderStage) + String(pesticideStage) + String(agitatorControl);
  writeData(stateMsg);
  Serial.print(F("> Publish State Message : "));
  Serial.println(stateMsg);
}

/**   ===================================================
                      Motor Controller
  =================================================== */

void powderController() {
  if (powderControl) {
    if (powderStage == 1) {
      // Moving Motor
      digitalWrite(powderMotorDIRPin, HIGH);
      powderMotor.step(stepRevolution * 4 * rotationSize);          // rotationSize 줄일 경우, 딜레이가 감소할 것으로 판단됨 - 테스트 필요
      delay(5);
      digitalWrite(powderMotorPWMPin, LOW);
      powderMotorCnt++;
      Serial.print(F("> (1) **Powder** Moter Rotation.."));
      Serial.println(powderMotorCnt);
      if (powderMotorCnt == powderMotorMoving) {
        powderStage = 2;
      }
    } else if (powderStage == 2) {
      // Spray Water
      long mSec = millis();
      if (powderSprayStart == 0) {
        powderSprayStart = mSec;
        digitalWrite(powderValvePin, HIGH);
        Serial.println(F("> (1) Start **Powder** Water Spray.."));
      }
      if (mSec - powderSprayStart >= powderSprayTime) {
        powderSprayStart = 0;
        powderStage = 3;
        digitalWrite(powderValvePin, LOW);
        Serial.println(F("> (1) Stop **Powder** Water Spray.."));
      }
    } else if (powderStage == 3) {
      // Moving Return
      returnPowderMotor();
    }
  } else {
    digitalWrite(powderValvePin, LOW);
    digitalWrite(powderMotorPWMPin, LOW);
    returnPowderMotor();
  }
}

void returnPowderMotor() {
  if (powderMotorCnt == 0) {
    powderStage = 0;
    powderControl = 0;
  } else {
    powderStage = 3;
    digitalWrite(powderMotorDIRPin, LOW);
    powderMotor.step(stepRevolution * 4 * rotationSize);
    delay(5);
    digitalWrite(powderMotorPWMPin, LOW);
    powderMotorCnt--;
    Serial.print("> (1) **Powder** Moter Return..");
    Serial.println(powderMotorCnt);
  }
}

void pesticideController() {
  if (pesticideControl) {
    if (pesticideStage == 1) {
      // Moving Motor
      digitalWrite(pesticideMotorDIRPin, HIGH);
      pesticideMotor.step(stepRevolution * 4 * rotationSize);
      delay(5);
      digitalWrite(pesticideMotorPWMPin, LOW);
      pesticideMotorCnt++;
      Serial.print("> (2) **Pesticide** Moter Rotation..");
      Serial.println(pesticideMotorCnt);
      if (pesticideMotorCnt == pesticideMotorMoving) {
        pesticideStage = 2;
      }
    } else if (pesticideStage == 2) {
      // Spray Water
      long mSec = millis();
      if (pesticideSprayStart == 0) {
        pesticideSprayStart = mSec;
        digitalWrite(pesticideValvePin, HIGH);
        Serial.println(F("> (2) Start **Pesticide** Water Spray.."));
      }
      if (mSec - pesticideSprayStart >= pesticideSprayTime) {
        pesticideSprayStart = 0;
        pesticideStage = 3;
        digitalWrite(pesticideValvePin, LOW);
        Serial.println(F("> (2) Stop **Pesticide** Water Spray.."));
      }
    } else if (pesticideStage == 3) {
      // Moving Return
      returnPesticideMotor();
    }
  } else {
    digitalWrite(pesticideValvePin, LOW);
    digitalWrite(pesticideMotorPWMPin, LOW);
    returnPesticideMotor();
  }
}

void returnPesticideMotor() {
  if (pesticideMotorCnt == 0) {
    pesticideStage = 0;
    pesticideControl = 0;
  } else {
    pesticideStage = 3;
    digitalWrite(pesticideMotorDIRPin, LOW);
    pesticideMotor.step(stepRevolution * 4 * rotationSize);
    delay(5);
    digitalWrite(pesticideMotorPWMPin, LOW);
    pesticideMotorCnt--;
    Serial.print("> (2) **Pesticide** Moter Return..");
    Serial.println(pesticideMotorCnt);
  }
}

void agitatorController() {
  if (agitatorControl) {
    long mSec = millis();
    if (agitatorStart == 0) {
      agitatorStart = mSec;
      digitalWrite(agitatorMotorPin, HIGH);
      Serial.println(F("> (3) Start **Agitator** Motor.."));
    }
    if (mSec - agitatorStart >= agitatorTime) {
      agitatorControl = 0;
      agitatorStart = 0;
      digitalWrite(agitatorMotorPin, LOW);
      Serial.println(F("> (3) Stop **Agitator** Motor.."));
    }
  } else {
    digitalWrite(agitatorMotorPin, LOW);
  }
}

/**   ===================================================
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
}

void messageHandler() {
  if (recvData.equals(RECV_WIFI_DISCONNECT)) {
    mqttPassable = false;
    Serial.println(F("# Wifi Disconnected.."));
  } else if (recvData.equals(RECV_CONN_MQTT)) {
    mqttPassable = true;
    Serial.println(F("# Mqtt Connected.."));
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
  powderControl = String(recvData[2]).toInt();
  pesticideControl = String(recvData[3]).toInt();
  agitatorControl = String(recvData[4]).toInt();

  if (powderControl) {
    powderStage = powderControl;
  }

  if (pesticideControl) {
    pesticideStage = pesticideControl;
  }

  agitatorStart = powderSprayStart = pesticideSprayStart = 0;
  pushStateTime = 0;

  Serial.print("# Control Order >> Powder = ");
  Serial.print(powderControl);
  Serial.print(", Pesticide = ");
  Serial.print(pesticideControl);
  Serial.print(", Agitator = ");
  Serial.println(agitatorControl);
}

void setSettingValues() {
  recvData = recvData.substring(2, recvData.length());
  String infos[6];

  int position = 0;
  while (true)
  {
    int sepIndex = recvData.indexOf(",");
    if (sepIndex != -1)
    {
      infos[position++] = recvData.substring(0, sepIndex);
      recvData = recvData.substring(sepIndex + 1);
    }
    else
    {
      infos[position] = recvData;
      break;
    }
  }

  powderMotorMoving = infos[0].toInt();
  powderSprayTime = infos[1].toInt() * 1000;
  pesticideMotorMoving = infos[2].toInt();
  pesticideSprayTime = infos[3].toInt() * 1000;
  agitatorTime = infos[4].toInt() * 1000;

  Serial.print("# Setting Info : ");
  Serial.print(powderMotorMoving);
  Serial.print(", ");
  Serial.print(powderSprayTime);
  Serial.print(", ");
  Serial.print(pesticideMotorMoving);
  Serial.print(", ");
  Serial.print(pesticideSprayTime);
  Serial.print(", ");
  Serial.println(agitatorTime);
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
