#ifndef HSC_BASE_H
#define HSC_BASE_H

#include "ConfigManager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>

// Forward declaration
class HSC_Base;

class HSC_Base {
public:
  HSC_Base();
  void begin();
  void loop();

  // Set Board Info
  void setBoardInfo(const char *desc, const char *shortName);

  // Register a custom page handler
  void registerPage(const char *uri, ArRequestHandlerFunction handler);

  // Register a custom API handler
  void registerApi(const char *uri, WebRequestMethodComposite method,
                   ArRequestHandlerFunction handler);

  // Getters
  AsyncWebServer &getServer() { return server; }
  PubSubClient &getMqttClient() { return mqttClient; }
  Config &getConfig() { return currentConfig; }

  // Get the template processor function
  String processTemplate(const String &var) { return processor(var); }

private:
  AsyncWebServer server;
  WiFiClient espClient;
  PubSubClient mqttClient;
  ConfigManager configManager;
  Config currentConfig;

  bool shouldReboot = false;
  bool locateActive = false;
  unsigned long lastMqttReconnectAttempt = 0;
  String boardTypeDesc;
  String boardTypeShort;

  void setupWifi();
  void reconnectMqtt();
  void setupWebServer();
  String processor(const String &var);
};

#endif
