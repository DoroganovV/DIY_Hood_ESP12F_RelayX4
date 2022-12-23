#include <GyverPortal.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <LittleFS.h>

struct Device {
  char ssid[20];
  char pass[20];
  char host[20];
  int16_t port;
  char topicLightGet[50];
  char topicLightSet[50];
  char topicModeGet[50];
  char topicModeSet[50];
};
struct Values {
  byte SpeedHood = 0;
  bool Light = false;
  byte SpeedHoodLast = 0;
  bool LightLast = false;
  byte SpeedHoodHard = 0;
  bool LightHard = false;
};

const byte pinLight = 16;
const byte pinMode1 = 14;
const byte pinMode2 = 12;
const byte pinMode3 = 13;

const byte pinInLight = 5;
const byte pinInMode1 = 4;
const byte pinInMode2 = 0;
const byte pinInMode3 = 2;

const String version = "2.0.0";

byte mode = 0;

Device device;
Values values;
GyverPortal ui(&LittleFS);
WiFiClient espClient;
PubSubClient client(espClient);

void build() {
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);
  GP.UI_BEGIN("Smart Hood", "/,/wifi,/mqtt,/about", "Control,WiFi,MQTT,About");
  GP.PAGE_TITLE("Smart Hood " + version);
  GP.ONLINE_CHECK();
  if (ui.uri("/")) {
    /*if (mode > 0) {
      GP.JQ_SUPPORT();
      GP.JQ_UPDATE_BEGIN();
    }*/
    GP.UPDATE("spdPid,lightOn");
    M_BLOCK_TAB("SPEED",
      M_BOX(GP.LABEL("Speed"); GP.SLIDER("spdPid", values.SpeedHood, 0, 3); ); //GP.SLIDER_C
      M_BOX(GP.BUTTON("downSpeed", "<<"); GP.BUTTON("upSpeed", ">>"););
    );
    M_BLOCK_TAB("Light",
      GP.SWITCH("lightOn", values.Light); //GP.BREAK();
    );
    /*if (mode > 0) {
      GP.JQ_UPDATE_END();
    }*/
  } else if (ui.uri("/wifi")) {
    GP.FORM_BEGIN("/wifi");
    M_BLOCK_TAB("WiFi",
      M_BOX(GP.LABEL("SSID"); GP.TEXT("lg", "SSID", device.ssid););
      M_BOX(GP.LABEL("Password"); GP.PASS("ps", "Password", device.pass););
      GP.SUBMIT("Save");
    );
    GP.FORM_END();
  } else if (ui.uri("/mqtt")) {
    GP.FORM_BEGIN("/mqtt");
    M_BLOCK_TAB("MQTT",
      M_BOX(GP.LABEL("Host"); GP.TEXT("hst", "192.168.1.11", device.host););
      M_BOX(GP.LABEL("Port"); GP.NUMBER("pt", "1883", device.port););
      GP.HR();
      M_BOX(GP.LABEL("Topic light get"); GP.TEXT("lg", "/devices/hood/controls/Light", device.topicLightGet););
      M_BOX(GP.LABEL("Topic light set"); GP.TEXT("ls", "/devices/hood/controls/Light/on", device.topicLightSet););
      M_BOX(GP.LABEL("Topic mode get");  GP.TEXT("mg", "/devices/hood/controls/Mode", device.topicModeGet););
      M_BOX(GP.LABEL("Topic mode set");  GP.TEXT("ms", "/devices/hood/controls/Mode/on", device.topicModeSet););
      GP.SUBMIT("Save");
    );
    GP.FORM_END();
  } else if (ui.uri("/about")) {
    GP.TITLE("Smart Hood " + version); GP.BREAK();
    GP.SPAN("SmartHood (WiFi + MQTT) is project for automatisation hood."); GP.BREAK();
    GP.HR();    
    GP.BUTTON_LINK("https://www.esphome-devices.com/devices/ESP-12F-Relay-X4", "Hardware ESP12F_Relay_X4_V1.2");
    GP.BUTTON_LINK("https://github.com/GyverLibs/GyverPortal", "UI GyverPortal");
    GP.BUTTON_LINK("https://github.com/DoroganovV", "Autor Vitaly Doroganov");
    GP.HR();
    GP.OTA_FIRMWARE("Upload firmware");
    GP.BUTTON("reset", "Reset settings to default");
    GP.HR();
    GP.SYSTEM_INFO(version);
  } 
  GP.BUILD_END();
}

void pinSetup() {
  pinMode(pinLight, OUTPUT);
  pinMode(pinMode1, OUTPUT);
  pinMode(pinMode2, OUTPUT);
  pinMode(pinMode3, OUTPUT);
  
  pinMode(pinInLight, INPUT_PULLUP);
  pinMode(pinInMode1, INPUT_PULLUP);
  pinMode(pinInMode2, INPUT_PULLUP);
  pinMode(pinInMode3, INPUT_PULLUP);

  checkInputs();
}

void portalSetup() {
  if (!LittleFS.begin()) Serial.println("FS Error");
  Serial.println("Portal start");
  ui.attachBuild(build);
  ui.start(WIFI_AP);
  ui.attach(action);
  //ui.enableOTA();
  //ui.downloadAuto(true);
}

void updateOutputs(bool isNeedMqtt)
{
  digitalWrite(pinLight, values.Light ? HIGH : LOW);
  digitalWrite(pinMode1, values.SpeedHood == 1 ? HIGH : LOW);
  digitalWrite(pinMode2, values.SpeedHood == 2 ? HIGH : LOW);
  digitalWrite(pinMode3, values.SpeedHood == 3 ? HIGH : LOW);

  if (mode == 2 && isNeedMqtt && (values.SpeedHoodLast != values.SpeedHood || values.LightLast != values.Light)) {
    client.publish(device.topicLightSet, values.Light ? "0" : "1");
    client.publish(device.topicModeSet, String(values.SpeedHood).c_str());
  }

  values.SpeedHoodLast = values.SpeedHood;
  values.LightLast = values.Light;
}

void checkInputs()
{
  bool lightHardLast = !digitalRead(pinInLight);
  if (values.LightHard != lightHardLast) {
    values.Light = lightHardLast;
    Serial.print("valLight was changes to:"); Serial.println(values.Light);
  }

  byte speedHoodHardLast;
  if (!digitalRead(pinInMode1)) {
    speedHoodHardLast = 1;
  } else if (!digitalRead(pinInMode2)) {
    speedHoodHardLast = 2;
  } else if (!digitalRead(pinInMode3)) {
    speedHoodHardLast = 3;
  } else {
    speedHoodHardLast = 0;
  }  
  if (values.SpeedHoodHard != speedHoodHardLast) {
    values.SpeedHood = speedHoodHardLast;
    Serial.print("SpeedHood was changes to:"); Serial.println(values.SpeedHood);
  }
  
  values.LightHard = lightHardLast;
  values.SpeedHoodHard = speedHoodHardLast;
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();

  pinSetup();
  // читаем данные из памяти
  EEPROM.begin(300);
  EEPROM.get(0, device);

  if (strlen(device.ssid) == 0 || strlen(device.pass) == 0) {
    mode = 0;
  } else {
    Serial.print("Connecting to WiFi ("); Serial.print(device.ssid); Serial.println("):");
    WiFi.mode(WIFI_STA);
    WiFi.begin(device.ssid, device.pass);
    for (int i = 0; i < 10; i++) {
      delay(1000);
      Serial.print(".");
      if (WiFi.status() == WL_CONNECTED) {
        i = 10;
        mode = 1;
      }
    }
    if (mode == 1 && strlen(device.host) > 0) {
      Serial.print("Connecting to MQTT ("); Serial.print(device.host); Serial.println("):");
      client.setServer(device.host, device.port);
      for (int i = 0; i < 10; i++) {
        delay(1000);
        Serial.print(".");
        if (client.connected()) {
          i = 10;
          mode = 2;
        }
      }
    }
  }
  Serial.print("Mode:"); Serial.println(mode);

  if (mode == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WiFi Config");
  } 

  if (mode > 0) {
    Serial.println();
    Serial.print("Connected! Local IP: ");
    Serial.println(WiFi.localIP());
  }
  
  if (mode > 1) {
    Serial.println("connected");
    client.setCallback(callback);
    client.subscribe(device.topicLightGet);
    client.subscribe(device.topicModeGet);
  }
  
  portalSetup();
  updateOutputs(true);
}

void action(GyverPortal& p) {
  if (p.form("/wifi")) {
    Serial.println("save wifi");
    p.copyStr("lg", device.ssid);
    p.copyStr("ps", device.pass);
    EEPROM.put(0, device);
    EEPROM.commit();
    delay(5000);
    ESP.restart();
  }

  if (p.form("/mqtt")) {
    Serial.println("save mqtt");
    p.copyStr("hst", device.host);
    p.copyInt("pt", device.port);
    p.copyStr("lg", device.topicLightGet);
    p.copyStr("ls", device.topicLightSet);
    p.copyStr("mg", device.topicModeGet);
    p.copyStr("ms", device.topicModeSet);
    EEPROM.put(0, device);
    EEPROM.commit();
    delay(5000);
    ESP.restart();
  }
  
  if (ui.click()) {
    if (ui.clickInt("spdPid", values.SpeedHood)) {
      Serial.print("Speed: "); Serial.println(values.SpeedHood);
    }

    if (ui.clickBool("lightOn", values.Light)) {
      Serial.print("Switch: "); Serial.println(values.Light);
    }

    if (ui.click("downSpeed")) {
      Serial.println("Down speed click");
      if (values.SpeedHood > 0)
        values.SpeedHood--;
    }
    
    if (ui.click("upSpeed")) {
      Serial.println("Up speed click");
      if (values.SpeedHood < 3)
        values.SpeedHood++;
    }

    if (ui.click("reset")) {
      strcpy(device.ssid, "");
      strcpy(device.pass, "");
      strcpy(device.host, "");
      device.port = 1883;
      strcpy(device.topicLightGet, "/devices/hood/controls/Light");
      strcpy(device.topicLightSet, "/devices/hood/controls/Light/on");
      strcpy(device.topicModeGet, "/devices/hood/controls/Mode");
      strcpy(device.topicModeSet, "/devices/hood/controls/Mode/on");
      EEPROM.put(0, device);
      EEPROM.commit();
      delay(5000);
      ESP.restart();
    }
  }
  
  if (ui.update()) {
    if (ui.update("spdPid")) ui.answer(values.SpeedHood);
    if (ui.update("lightOn")) ui.answer(values.Light);
  }

  updateOutputs(true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String strTopic = String(topic);
  String strPayload = "";
  
  Serial.println(strTopic);    
  for (int i = 0; i < length; i++)
    strPayload += (char)payload[i];    
  Serial.println(strPayload);
  
  if (strTopic == device.topicLightGet) {
    values.Light = strPayload == "1";
  } else if (strTopic == device.topicModeGet) {
    values.SpeedHood = strPayload == "1" ? 1 : 
                   strPayload == "2" ? 2 :
                   strPayload == "3" ? 3 : 0;
  }

  updateOutputs(false);
}

void loop() {
  ui.tick();
  checkInputs();
  updateOutputs(true);
}
