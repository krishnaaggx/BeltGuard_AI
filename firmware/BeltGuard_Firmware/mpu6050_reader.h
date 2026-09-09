#ifndef MPU6050_READER_H
#define MPU6050_READER_H

#include <Wire.h>

#define MPU_ADDR 0x68

bool mpuReady = false;

void writeMPU(byte reg, byte val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

void initMPU6050() {
  delay(100);

  // Wake up — register 0x6B = 0x00
  writeMPU(0x6B, 0x00);
  delay(50);

  // Verify device responds
  Wire.beginTransmission(MPU_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
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
  float crest_factor;  // peak/rms ratio — ML feature for fault detection
                       // Normal belt: ~1.4 | Early fault: 3-5 | Severe: >6
  bool fault;
};

VibrationData readVibration() {
  VibrationData result = {0.0, 0.0, 0.0, false};

  if (!mpuReady) {
    result.fault = true;
    return result;
  }

  const int SAMPLES = 100;
  float sumSquares = 0.0;
  float peak = 0.0;

  for (int i = 0; i < SAMPLES; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // ACCEL_XOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);

    int16_t ax = Wire.read() << 8 | Wire.read();
    int16_t ay = Wire.read() << 8 | Wire.read();
    int16_t az = Wire.read() << 8 | Wire.read();

    float ax_g = ax / 16384.0;
    float ay_g = ay / 16384.0;
    float az_g = az / 16384.0;

    float magnitude = sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
    sumSquares += magnitude * magnitude;
    if (magnitude > peak) peak = magnitude;

    delayMicroseconds(500); // 2kHz sampling
  }

  result.rms  = sqrt(sumSquares / SAMPLES);
  result.peak = peak;

  // Crest factor — avoid divide-by-zero
  result.crest_factor = (result.rms > 0.001) ? (result.peak / result.rms) : 0.0;

  result.fault = false;
  return result;
}

#endif
