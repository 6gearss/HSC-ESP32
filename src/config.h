#ifndef CONFIG_H
#define CONFIG_H

// --- General Configuration ---
static const char FW_VERSION[] = "0.1.0"; // Base Template

// --- WiFi Configuration ---
static const char *WIFI_SSID = "LocoNet";
static const char *WIFI_PASSWORD = "MyTrainRoom";

// --- MQTT Configuration ---
static const char *MQTT_SERVER = "mqtt.internal";
static const int MQTT_PORT = 1883;
static const char *MQTT_USER = "";     // Leave empty if not needed
static const char *MQTT_PASSWORD = ""; // Leave empty if not needed

// --- Device Configuration ---
// CHANGE THIS ID FOR EACH BOARD
static const int BOARD_ID = 0;

// --- Pin Definitions ---
// AP Mode Button
static const int PIN_AP_BUTTON = 4;

#endif
