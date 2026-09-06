#ifndef LOAD_CELL_READER_H
#define LOAD_CELL_READER_H

#include <HX711.h>

#define HX711_DOUT_PIN 16
#define HX711_SCK_PIN  17

HX711 scale;
bool hx711Ready = false;

// calibration factor — you MUST update this after physical calibration
// place a known weight, adjust this number until reading matches
#define CALIBRATION_FACTOR -7050.0

void initLoadCell() {
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  if (scale.is_ready()) {
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare(); // reset to zero on boot
    hx711Ready = true;
    Serial.println("HX711 connected.");
  } else {
    Serial.println("HX711 FAILED — check wiring.");
    hx711Ready = false;
  }
}

struct LoadData {
  float load_kg;
  bool fault;
};

LoadData readLoad() {
  LoadData result = {0.0, false};

  if (!hx711Ready) {
    result.fault = true;
    return result;
  }

  if (!scale.is_ready()) {
    result.fault = true;
    return result;
  }

  float reading = scale.get_units(5); // average of 5 readings
  
  // sanity check — reject clearly wrong values
  if (isnan(reading) || reading < -10.0 || reading > 5000.0) {
    result.fault = true;
    return result;
  }

  result.load_kg = abs(reading); // some setups return negative
  return result;
}

#endif