#ifndef HALL_SENSOR_H
#define HALL_SENSOR_H

#define HALL_PIN 34  // change GPIO if needed — must be input-capable

volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;
volatile int totalPulseCount = 0;
bool hallReady = false;

void IRAM_ATTR hallISR() {
  unsigned long now = micros();
  if (lastPulseTime > 0) {
    pulseInterval = now - lastPulseTime;
  }
  lastPulseTime = now;
  totalPulseCount++;
}

void initHallSensor() {
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, FALLING);
  hallReady = true;
  Serial.println("Hall effect sensor initialized.");
}

struct HallData {
  float belt_speed_mps;
  String joint_id;
  bool fault;
};

// magnet_spacing_m = distance between magnets on belt
// one magnet per joint — so this equals belt circumference / total_joints
HallData readHall(float magnet_spacing_m = 1.0) {
  HallData result = {0.0, "J-00", false};

  if (!hallReady) {
    result.fault = true;
    return result;
  }

  // if no pulse in last 3 seconds — belt stopped or sensor fault
  unsigned long timeSinceLastPulse = micros() - lastPulseTime;
  if (lastPulseTime == 0 || timeSinceLastPulse > 3000000) {
    result.belt_speed_mps = 0.0;
    result.joint_id = "J-00";
    result.fault = (lastPulseTime == 0); // fault only if never got a pulse
    return result;
  }

  // speed = distance / time
  noInterrupts();
  unsigned long interval = pulseInterval;
  int pulseCount = totalPulseCount;
  interrupts();

  if (interval > 0) {
    result.belt_speed_mps = magnet_spacing_m / (interval / 1000000.0);
  }

  // joint ID = which splice is currently passing
  int jointIndex = (pulseCount % TOTAL_JOINTS) + 1;
  result.joint_id = "J-0" + String(jointIndex);

  return result;
}

#endif