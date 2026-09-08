#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "mpu6050_reader.h"
#include "temperature_reader.h"
#include "hall_sensor.h"
#include "load_cell_reader.h"
#include "ir_array_reader.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ─── WiFi ───────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

// ─── MQTT ───────────────────────────────────────────
void connectMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 3s");
      delay(3000);
    }
  }
}

// ─── Publish test payload ────────────────────────────
void publishPayload() {
  StaticJsonDocument<512> doc;

  // Identity
  doc["node_id"]   = NODE_ID;
  doc["timestamp_ms"] = millis();

  // DUMMY sensor values — replace later with real reads
  VibrationData vib = readVibration();
  doc["vibration_rms"]   = vib.rms;
  doc["vibration_peak"]  = vib.peak;
  if (vib.fault) 
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 0);
  TemperatureData temps = readTemperatures();
  doc["temp_surface_c"] = temps.surface_c;
  doc["temp_bearing_c"] = temps.bearing_c;
  if (temps.mlxFault) 
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 1);
  if (temps.dsFault)  
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 2);
  HallData hall = readHall(1.0);
  doc["belt_speed_mps"] = hall.belt_speed_mps;
  doc["joint_id"]       = hall.joint_id;
  if (hall.fault) 
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 4);
  LoadData load = readLoad();
  doc["load_kg"] = load.load_kg;
  if (load.fault) 
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 3);
  IRData ir = readIRArray();
  doc["ir_offset_mm"] = ir.offset_mm;
  if (ir.fault) 
    doc["sensor_fault_flags"] = doc["sensor_fault_flags"].as<int>() | (1 << 5);
  doc["anomaly_score"]   = 0.0;   // backend will compute real value
  doc["risk_level"]      = "NORMAL";

  char buffer[512];
  serializeJson(doc, buffer);

  String topic = "beltguard/node/" + String(NODE_ID) + "/data";
  mqttClient.publish(topic.c_str(), buffer);

  Serial.print("Published: ");
  Serial.println(buffer);
}

// ─── Setup ──────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  initMPU6050();
  initTemperatureSensors();
  initHallSensor();
  initLoadCell();
  initIRArray();
  connectWiFi();
  connectMQTT();
}

// ─── Loop ───────────────────────────────────────────
void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  publishPayload();
  delay(2000);  // publish every 2 seconds
}