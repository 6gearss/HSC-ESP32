#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config.h"
#include <Arduino.h>
#include <Preferences.h>

struct Config {
  String wifi_ssid;
  String wifi_password;
  String mqtt_server;
  int mqtt_port;
  String mqtt_user;
  String mqtt_password;
  int board_id;
  String location;
};

class ConfigManager {
public:
  ConfigManager();
  bool begin();
  Config load();
  bool save(const Config &config);
  void reset();
  Config get() const { return _config; }

private:
  Config _config;
  Preferences _prefs;
  void loadDefaults();
};

#endif
