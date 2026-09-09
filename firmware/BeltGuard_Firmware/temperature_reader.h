#ifndef TEMPERATURE_READER_H
#define TEMPERATURE_READER_H

#include <Adafruit_MLX90614.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "pins.h" // change this GPIO pin if needed

Adafruit_MLX90614 mlx;
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

bool mlxReady = false;
bool dsReady  = false;

void initTemperatureSensors() {
  // MLX90614 — HARDWARE FAILURE, REMOVED
  mlxReady = false;
  Serial.println("MLX90614 skipped — not available.");

  // DS18B20
  ds18b20.begin();
  if (ds18b20.getDeviceCount() > 0) {
    Serial.println("DS18B20 connected.");
    dsReady = true;
  } else {
    Serial.println("DS18B20 FAILED — check wiring.");
    dsReady = false;
  }
}

struct TemperatureData {
  float surface_c;   // MLX90614 — splice surface
  float bearing_c;   // DS18B20  — idler bearing
  bool  mlxFault;
  bool  dsFault;
};

TemperatureData readTemperatures() {
  TemperatureData result = {0.0, 0.0, false, false};

  // MLX90614
  if (!mlxReady) {
    result.mlxFault = true;
  } else {
    float t = mlx.readObjectTempC();
    if (isnan(t) || t < -40.0 || t > 200.0) {
      result.mlxFault = true;
    } else {
      result.surface_c = t;
    }
  }

  // DS18B20
  if (!dsReady) {
    result.dsFault = true;
  } else {
    ds18b20.requestTemperatures();
    float t = ds18b20.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C || t < -40.0 || t > 150.0) {
      result.dsFault = true;
    } else {
      result.bearing_c = t;
    }
  }

  return result;
}

#endif