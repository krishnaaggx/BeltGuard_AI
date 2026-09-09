#ifndef CONFIG_H
#define CONFIG_H

// WiFi
#define WIFI_SSID     "ayush"
#define WIFI_PASSWORD "9990781237"

// MQTT Broker — Laptop IP on same network
// Run ipconfig on laptop to confirm this IP
#define MQTT_BROKER   "172.24.127.110"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "BeltGuard_NODE_01"

// Node identity
#define NODE_ID       "NODE_01"
#define TOTAL_JOINTS  4  // how many splices on your belt

// NTP — for real wall-clock timestamps (needed for ML time-series)
#define NTP_SERVER       "pool.ntp.org"
#define GMT_OFFSET_SEC   19800   // IST = UTC + 5:30 = 19800 seconds
#define DAYLIGHT_OFFSET  0

// ML Label — change manually when running fault simulation sessions
// Values: "NORMAL" | "MISALIGNMENT" | "OVERLOAD" | "BEARING_FAULT" | "SPLICE_FAULT"
#define BELT_LABEL    "NORMAL"

#endif
