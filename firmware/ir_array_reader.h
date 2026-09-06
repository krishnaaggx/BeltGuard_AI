#ifndef IR_ARRAY_READER_H
#define IR_ARRAY_READER_H

// Define your IR sensor GPIO pins
// Add or remove pins based on how many sensors you have
#define IR_SENSOR_COUNT 3
const int IR_PINS[IR_SENSOR_COUNT] = {32, 33, 35};

bool irReady = false;

void initIRArray() {
  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    pinMode(IR_PINS[i], INPUT);
  }
  irReady = true;
  Serial.println("IR array initialized.");
}

struct IRData {
  float offset_mm;   // estimated belt edge displacement
  bool fault;
};

IRData readIRArray() {
  IRData result = {0.0, false};

  if (!irReady) {
    result.fault = true;
    return result;
  }

  int readings[IR_SENSOR_COUNT];
  int triggeredCount = 0;
  int firstTriggered = -1;
  int lastTriggered  = -1;

  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    readings[i] = digitalRead(IR_PINS[i]);
    if (readings[i] == LOW) { // LOW = object detected for most IR sensors
      triggeredCount++;
      if (firstTriggered == -1) firstTriggered = i;
      lastTriggered = i;
    }
  }

  // no sensors triggered = belt missing or all sensors faulty
  if (triggeredCount == 0) {
    result.fault = true;
    return result;
  }

  // offset estimated from which sensors are triggered
  // center sensor = 0 offset, outer sensors = displacement
  // 10mm per sensor spacing — update this to your actual spacing
  float sensorSpacingMM = 10.0;
  float centerIndex = (IR_SENSOR_COUNT - 1) / 2.0;
  float triggeredCenter = (firstTriggered + lastTriggered) / 2.0;
  result.offset_mm = (triggeredCenter - centerIndex) * sensorSpacingMM;

  return result;
}

#endif