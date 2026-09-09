#ifndef PINS_H
#define PINS_H

// I2C Bus (MPU6050 + MLX90614)
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// DS18B20 (1-Wire, idler bearing temp)
#define DS18B20_PIN 26

// Hall Effect Sensor (joint indexing)
#define HALL_PIN 27

// HX711 (belt loading)
// NOTE: Changed from 16,17 to 4,5 — ESP32 DevKit V1 exposes these reliably
#define HX711_DOUT_PIN 4
#define HX711_SCK_PIN  5

// IR Sensor Array (belt alignment)
#define IR_SENSOR_COUNT 3
#define PIN_IR_1 25
#define PIN_IR_2 33
#define PIN_IR_3 19

#endif // PINS_H
