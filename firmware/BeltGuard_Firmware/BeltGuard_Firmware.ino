#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "pins.h"
#include "mpu6050_reader.h"
#include "temperature_reader.h"
#include "hall_sensor.h"
#include "load_cell_reader.h"
#include "ir_array_reader.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ─────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────

void connectWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected.");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ─────────────────────────────────────────────
// MQTT
// ─────────────────────────────────────────────

void connectMQTT() {

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  while (!mqttClient.connected()) {

    Serial.print("Connecting to MQTT...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {

      Serial.println("connected.");

    } else {

      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());

      Serial.println(" | retrying in 3 seconds");

      delay(3000);
    }
  }
}

// ─────────────────────────────────────────────
// Publish Sensor Data
// ─────────────────────────────────────────────

void publishPayload() {

  StaticJsonDocument<768> doc;

  // ─────────────────────────────────────────
  // Sensor fault bit mask
  //
  // Bit 0 = MPU6050
  // Bit 1 = MLX90614
  // Bit 2 = DS18B20
  // Bit 3 = HX711
  // Bit 4 = Hall sensor
  // Bit 5 = IR array
  // ─────────────────────────────────────────

  int sensorFaultFlags = 0;

  // ─────────────────────────────────────────
  // Identity
  // ─────────────────────────────────────────

  doc["node_id"] = NODE_ID;
  doc["timestamp_ms"] = millis();

  // ─────────────────────────────────────────
  // VIBRATION
  // ─────────────────────────────────────────

  VibrationData vib = readVibration();

  doc["vibration_rms"] = vib.rms;
  doc["vibration_peak"] = vib.peak;

  if (vib.fault) {
    sensorFaultFlags |= (1 << 0);
  }

  // ─────────────────────────────────────────
  // TEMPERATURE
  // ─────────────────────────────────────────

  TemperatureData temps = readTemperatures();

  doc["temp_surface_c"] = temps.surface_c;
  doc["temp_bearing_c"] = temps.bearing_c;

  if (temps.mlxFault) {
    sensorFaultFlags |= (1 << 1);
  }

  if (temps.dsFault) {
    sensorFaultFlags |= (1 << 2);
  }

  // ─────────────────────────────────────────
  // HALL SENSOR / BELT SPEED
  // ─────────────────────────────────────────

  HallData hall = readHall(1.0);

  doc["belt_speed_mps"] = hall.belt_speed_mps;
  doc["joint_id"] = hall.joint_id;

  if (hall.fault) {
    sensorFaultFlags |= (1 << 4);
  }

  // ─────────────────────────────────────────
  // LOAD CELL
  // ─────────────────────────────────────────

  LoadData load = readLoad();

  doc["load_kg"] = load.load_kg;

  if (load.fault) {
    sensorFaultFlags |= (1 << 3);
  }

  // ─────────────────────────────────────────
  // IR ARRAY
  // ─────────────────────────────────────────

  IRData ir = readIRArray();

  doc["ir_offset_mm"] = ir.offset_mm;

  if (ir.fault) {
    sensorFaultFlags |= (1 << 5);
  }

  // ─────────────────────────────────────────
  // SENSOR HEALTH
  // ─────────────────────────────────────────

  doc["sensor_fault_flags"] = sensorFaultFlags;

  doc["sensor_status"] =
      (sensorFaultFlags == 0) ? "OK" : "FAULT";

  // ─────────────────────────────────────────
  // IMPORTANT:
  //
  // ML MODEL WILL CALCULATE THESE.
  //
  // ESP32 only sends sensor data.
  // ─────────────────────────────────────────

  doc["anomaly_score"] = nullptr;
  doc["risk_level"] = nullptr;

  // ─────────────────────────────────────────
  // Serialize JSON
  // ─────────────────────────────────────────

  char buffer[768];

  serializeJson(doc, buffer);

  // ─────────────────────────────────────────
  // MQTT topic
  // ─────────────────────────────────────────

  String topic =
      "beltguard/node/" +
      String(NODE_ID) +
      "/data";

  bool success =
      mqttClient.publish(
          topic.c_str(),
          buffer
      );

  // ─────────────────────────────────────────
  // Serial output
  // ─────────────────────────────────────────

  Serial.println();
  Serial.println("────────────────────────────");

  if (success) {
    Serial.println("Sensor data published.");
  } else {
    Serial.println("MQTT publish FAILED.");
  }

  Serial.println(buffer);

  Serial.println("────────────────────────────");
}

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("      BELTGUARD SENSOR NODE");
  Serial.println("================================");

  // Initialize sensors

  initMPU6050();

  initTemperatureSensors();

  initHallSensor();

  initLoadCell();

  initIRArray();

  // Connect network

  connectWiFi();

  connectMQTT();
}

// ─────────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────────

void loop() {

  // Reconnect WiFi if required

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Reconnect MQTT if required

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  // Read and publish sensors

  publishPayload();

  // Publish every 2 seconds

  delay(2000);
}