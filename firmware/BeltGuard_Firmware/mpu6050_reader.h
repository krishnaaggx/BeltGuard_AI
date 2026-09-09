#ifndef MPU6050_READER_H
#define MPU6050_READER_H

#include <MPU6050.h>
#include "pins.h"

MPU6050 mpu;
bool mpuReady = false;

void initMPU6050() {
  Wire.begin();
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected.");
    mpuReady = true;
  } else {
    Serial.println("MPU6050 FAILED — check wiring.");
    mpuReady = false;
  }
}

struct VibrationData {
  float rms;
  float peak;
  bool fault;
};

VibrationData readVibration() {
  VibrationData result = {0.0, 0.0, false};

  if (!mpuReady) {
    result.fault = true;
    return result;
  }

  const int SAMPLES = 100;
  float sumSquares = 0.0;
  float peak = 0.0;

  for (int i = 0; i < SAMPLES; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // convert raw to g (range ±2g, sensitivity 16384 LSB/g)
    float ax_g = ax / 16384.0;
    float ay_g = ay / 16384.0;
    float az_g = az / 16384.0;

    // magnitude of acceleration vector
    float magnitude = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);

    sumSquares += magnitude * magnitude;
    if (magnitude > peak) peak = magnitude;

    delayMicroseconds(500); // 2kHz sampling
  }

  result.rms  = sqrt(sumSquares / SAMPLES);
  result.peak = peak;
  result.fault = false;
  return result;
}

#endif