#ifndef CONFIG_H
#define CONFIG_H

// WiFi
#define WIFI_SSID     "AMPEE"
#define WIFI_PASSWORD "Vinod@321"

// MQTT Broker — Charu's laptop IP on same network
#define MQTT_BROKER   "192.168.1.100"  // change to Charu's IP later
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "BeltGuard_NODE_01"

// Node identity
#define NODE_ID       "NODE_01"
#define TOTAL_JOINTS  4  // how many splices on your belt

#endif