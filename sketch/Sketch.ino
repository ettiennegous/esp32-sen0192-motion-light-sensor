#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include "Config.h"

#define SENSOR_NAME "ESP32-1"
#define MOTION_EVENT_NAME "Motion"
#define LUX_EVENT_NAME "Lux"

#define MOTION_SENSOR_PIN 35 /*D33 ESP32 DevKitV1*/

boolean flag = false;
boolean movement_detected = true;
String motion_event = String(SENSOR_NAME) + "/" + String(MOTION_EVENT_NAME);
String lux_event = String(SENSOR_NAME) + "/" + String(LUX_EVENT_NAME);
int threshold_lux_transmit = 2;
int previous_lux_transmit = 0;
int previous_lux_reading = 0;
bool flip_flop = false;
int connection_failure_count = 0;
int max_failure_count = 20;

TaskHandle_t MovementSensorThreadTask;

WiFiClient espClient;
PubSubClient client(espClient);
BH1750 lightMeter(0x23);

void setup() {
  Serial.begin(115200);
  delay(10);
  setup_mqtt();
  setupBH1750();

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
      flip_flop = true;
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Motion Detected");
      connect();
      client.publish(motion_event.c_str(), "true", true);
      client.loop();
      disconnect();
      delay(4000);
    }
    
    if(!movement_detected && flip_flop) {
      flip_flop = false;
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Motion Reset");
    }
    delay(200);
  }
}

void connect() {
  connect_wifi();
  connect_mqtt();
}

void disconnect() {
  disconnect_wifi();
  disconnect_mqtt();
}

/********************************** START MAIN LOOP***************************************/
void loop() {
  
  double lux = lightMeter.readLightLevel();
  if(lux != previous_lux_reading) {
    Serial.printf("Light  %lf lx\n", lux);
  }
  if(abs(lux - previous_lux_transmit) > threshold_lux_transmit) {
    connect();
    client.publish(lux_event.c_str(), ((String)lux).c_str(), true);
    previous_lux_transmit = lux;
    disconnect();
  }
  previous_lux_reading = lux;
  delay(2000);
}


void connect_wifi() {

  delay(10);
  Serial.println();
  Serial.println("Wifi Connecting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    restart_if_failing_lots();
    Serial.print(".");
    delay(500);
  }
  connection_failure_count = 0;
  Serial.println("\nWifi Connected");
}

void disconnect_wifi() {
  WiFi.disconnect();
}

void setup_mqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void connect_mqtt() {
  Serial.println("MQTT connecting");
  while (!client.connected()) {
    restart_if_failing_lots();
    Serial.print(".");
    if (client.connect(SENSOR_NAME, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("\nMQTT Connected");
    } 
    else {
      Serial.printf("\nFailed, rc= %s try again in 5 seconds\n", client.state());
      delay(5000);
    }
  }
  Serial.println("MQTT Complete");
  connection_failure_count = 0;
}

void disconnect_mqtt() {
  client.disconnect();
}

void restart_if_failing_lots() {
  if(connection_failure_count >= max_failure_count) {
    ESP.restart();
  }
  connection_failure_count++;
}
