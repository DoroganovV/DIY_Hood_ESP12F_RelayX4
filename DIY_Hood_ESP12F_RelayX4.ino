#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "{YOUR_SSID}";
const char* password = "{YOUR_PASSWORD}";
const char* mqtt_server = "{YOUR_MQTT_SERVER}"; //Сервер MQTT

const byte pinLight = 16;
const byte pinMode1 = 14;
const byte pinMode2 = 12;
const byte pinMode3 = 13;

const byte pinInLight = 5;
const byte pinInMode1 = 4;
const byte pinInMode2 = 0;
const byte pinInMode3 = 2;

bool lastPinInLightState = LOW;
bool lastPinInMode1State = LOW;
bool lastPinInMode2State = LOW;
bool lastPinInMode3State = LOW;

const char* topicNameLight = "/devices/wb-mr6c_52/controls/K4";
const char* topicNameLightOn = "/devices/wb-mr6c_52/controls/K4/on";
const char* topicNameMode = "/devices/hood/controls/Mode";
const char* topicNameModeOn = "/devices/hood/controls/Mode/on";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  randomSeed(micros());
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_MQTT() {
  digitalWrite(LED_BUILTIN, LOW);
  while (!client.connected()) {
    Serial.print("Connecting to MQTT: ");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topicNameLight);
      client.subscribe(topicNameMode);
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  digitalWrite(LED_BUILTIN, HIGH);
  String strTopic = String(topic);
  String strPayload = "";
  
  Serial.println(strTopic);    
  for (int i = 0; i < length; i++)
    strPayload += (char)payload[i];    
  Serial.println(strPayload);
  
  if (strTopic == topicNameLight) {
    digitalWrite(pinLight, strPayload == "1" ? HIGH : LOW);
  } else if (strTopic == topicNameMode) {
    digitalWrite(pinMode1, strPayload == "1" ? HIGH : LOW);
    digitalWrite(pinMode2, strPayload == "2" ? HIGH : LOW);
    digitalWrite(pinMode3, strPayload == "3" ? HIGH : LOW);
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void manualPinProcessing() {
  bool pinInLightState = digitalRead(pinInLight);
  if (pinInLightState != lastPinInLightState) 
  {
    Serial.print("pinInLightState: ");
    Serial.print(lastPinInLightState);
    Serial.print(" => ");
    Serial.println(pinInLightState);
    client.publish(topicNameLightOn, pinInLightState ? "0" : "1");
  }
  
  bool pinInMode1State = digitalRead(pinInMode1);
  bool pinInMode2State = digitalRead(pinInMode2);
  bool pinInMode3State = digitalRead(pinInMode3);
  if (pinInMode1State != lastPinInMode1State || pinInMode2State != lastPinInMode2State || pinInMode3State != lastPinInMode3State)
  {
    Serial.print("pinInModeState: ");
    Serial.print(lastPinInMode1State);
    Serial.print(lastPinInMode2State);
    Serial.print(lastPinInMode3State);
    Serial.print(" => ");
    Serial.print(pinInMode1State);
    Serial.print(pinInMode2State);
    Serial.println(pinInMode3State);
    
    if (!pinInMode1State) 
      client.publish(topicNameModeOn, "1");
    else if (!pinInMode2State) 
      client.publish(topicNameModeOn, "2");
    else if (!pinInMode3State) 
      client.publish(topicNameModeOn, "3");
    else
      client.publish(topicNameModeOn, "0");
  }
  lastPinInLightState = pinInLightState;
  lastPinInMode1State = pinInMode1State;
  lastPinInMode2State = pinInMode2State;
  lastPinInMode3State = pinInMode3State;
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(pinLight, OUTPUT);
  pinMode(pinMode1, OUTPUT);
  pinMode(pinMode2, OUTPUT);
  pinMode(pinMode3, OUTPUT);
  
  pinMode(pinInLight, INPUT_PULLUP);
  pinMode(pinInMode1, INPUT_PULLUP);
  pinMode(pinInMode2, INPUT_PULLUP);
  pinMode(pinInMode3, INPUT_PULLUP);
  
  digitalWrite(pinLight, LOW);
  digitalWrite(pinMode1, LOW);
  digitalWrite(pinMode2, LOW);
  digitalWrite(pinMode3, LOW);
  
  lastPinInLightState = digitalRead(pinInLight);
  lastPinInMode1State = digitalRead(pinInMode1);
  lastPinInMode2State = digitalRead(pinInMode2);
  lastPinInMode3State = digitalRead(pinInMode3);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    setup_MQTT();
  }
  manualPinProcessing();
  
  delay(500);
  client.loop();
}
