// pins.h
// BeltGuard AI — ESP32 DevKit V1
// Central pin definitions — DO NOT define pins anywhere else

#ifndef PINS_H
#define PINS_H

// ── I2C Bus (MPU6050 + MLX90614) ──────────────────────────
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22

// ── DS18B20 (1-Wire, idler bearing temp) ──────────────────
#define PIN_DS18B20        26

// ── Hall Effect Sensor (joint indexing) ───────────────────
#define PIN_HALL           27

// ── HX711 (belt loading) ──────────────────────────────────
#define PIN_HX711_DOUT     16
#define PIN_HX711_SCK      17

// ── IR Sensor Array (belt alignment) ──────────────────────
// NOT CONNECTED YET — reserve pins, do not use elsewhere
#define PIN_IR_1           25
#define PIN_IR_2           33
#define PIN_IR_3           19

// ── Pins intentionally left unused ───────────────────────
// GPIO12 — boot strapping, never use
// GPIO5, 14 — PWM noise at boot, avoided
// GPIO6-11 — internal flash, never touch

#endif // PINS_H