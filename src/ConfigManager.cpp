#include "ConfigManager.h"

ConfigManager::ConfigManager() { loadDefaults(); }

bool ConfigManager::begin() {
  // NVS doesn't need initialization like SPIFFS
  return true;
}

void ConfigManager::loadDefaults() {
  _config.wifi_ssid = WIFI_SSID;
  _config.wifi_password = WIFI_PASSWORD;
  _config.mqtt_server = MQTT_SERVER;
  _config.mqtt_port = MQTT_PORT;
  _config.mqtt_user = MQTT_USER;
  _config.mqtt_password = MQTT_PASSWORD;
  _config.board_id = BOARD_ID;
  _config.location = "";
  _config.location = "";
}

Config ConfigManager::load() {
  _prefs.begin("yarddetector", true); // Read-only mode

  // Check if config exists (board_id will be set if configured)
  if (!_prefs.isKey("board_id")) {
    Serial.println("No config found in NVS, using defaults");
    _prefs.end();
    loadDefaults();
    return _config;
  }

  // Load all values from NVS
  _config.wifi_ssid = _prefs.getString("wifi_ssid", WIFI_SSID);
  _config.wifi_password = _prefs.getString("wifi_pass", WIFI_PASSWORD);
  _config.mqtt_server = _prefs.getString("mqtt_srv", MQTT_SERVER);
  _config.mqtt_port = _prefs.getInt("mqtt_port", MQTT_PORT);
  _config.mqtt_user = _prefs.getString("mqtt_user", MQTT_USER);
  _config.mqtt_password = _prefs.getString("mqtt_pass", MQTT_PASSWORD);
  _config.board_id = _prefs.getInt("board_id", BOARD_ID);
  _config.location = _prefs.getString("location", "");
  _config.location = _prefs.getString("location", "");

  _prefs.end();

  Serial.println("Config loaded from NVS");
  return _config;
}

bool ConfigManager::save(const Config &config) {
  _prefs.begin("yarddetector", false); // Read-write mode

  // Save all values to NVS
  _prefs.putString("wifi_ssid", config.wifi_ssid);
  _prefs.putString("wifi_pass", config.wifi_password);
  _prefs.putString("mqtt_srv", config.mqtt_server);
  _prefs.putInt("mqtt_port", config.mqtt_port);
  _prefs.putString("mqtt_user", config.mqtt_user);
  _prefs.putString("mqtt_pass", config.mqtt_password);
  _prefs.putInt("board_id", config.board_id);
  _prefs.putString("location", config.location);
  _prefs.putString("location", config.location);

  _prefs.end();

  _config = config;
  Serial.println("Config saved to NVS");
  return true;
}

void ConfigManager::reset() {
  _prefs.begin("yarddetector", false);
  _prefs.clear(); // Clear all keys in this namespace
  _prefs.end();

  loadDefaults();
  Serial.println("Config reset to defaults");
}
