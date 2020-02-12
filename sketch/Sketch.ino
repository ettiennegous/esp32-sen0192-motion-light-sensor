#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include "FS.h"
#include "Config.h"
/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/

#define SENSOR_NAME "ESP32-1"
#define MOTION_EVENT_NAME "Motion"
#define LUX_EVENT_NAME "Lux"

/**************************** PIN DEFINITIONS ********************************************/
#define MOTION_SENSOR_PIN 35 /*D33 ESP32 DevKitV1*/

boolean flag = false;
boolean movement_detected = true;
String motion_event = String(SENSOR_NAME) + "/" + String(MOTION_EVENT_NAME);
String lux_event = String(SENSOR_NAME) + "/" + String(LUX_EVENT_NAME);

TaskHandle_t MovementSensorThreadTask;


WiFiClient espClient;
PubSubClient client(espClient);
BH1750 lightMeter(0x23);

void setup() {
  Serial.begin(115200);
  delay(10);

  /*setup_wifi();
  setup_mqtt();
  connect_mqtt();
  setupBH1750();*/

  pinMode(MOTION_SENSOR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  xTaskCreatePinnedToCore(
        MovementSensorThread, /* Function to implement the task */
        "MovementSensorThreadTask", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        0,  /* Priority of the task */
        &MovementSensorThreadTask,  /* Task handle. */
        0); /* Core where the task should run */
}

void setupBH1750() {
  Wire.begin(21,22);
  lightMeter.begin();
}

void MovementSensorThread(void * parameter) {
  for(;;) {
    movement_detected = !(digitalRead(MOTION_SENSOR_PIN));
    if((movement_detected)) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Motion Detected");
      client.publish(motion_event.c_str(), "true", true);
      client.loop();
      delay(4000);
    }
    
    if(!movement_detected) {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Motion Reset");
    }
    delay(10);
  }
}

/********************************** START MAIN LOOP***************************************/
void loop() {
  /*
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  client.publish(lux_event.c_str(), ((String)lux).c_str(), true);
  */
  delay(4000);
}


void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void teardown_wifi() {
  WiFi.disconnect();
}

void setup_mqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void connect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSOR_NAME, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void disconnect_mqtt() {
  client.disconnect();
}
