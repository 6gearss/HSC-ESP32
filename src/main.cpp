#include "ConfigManager.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h> // Added for JSON handling in API endpoints
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <cstdio> // For sprintf - satisfies linter
#include <time.h>

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);
ConfigManager configManager;
Config currentConfig;
bool locateActive = false; // Runtime only, not persisted
bool shouldReboot = false; // Flag to trigger reboot from loop

String processor(const String &var) {
  if (var == "FW_REV") {
    return FW_VERSION;
  }
  if (var == "IP") {
    if (WiFi.status() == WL_CONNECTED) {
      return WiFi.localIP().toString();
    } else {
      return WiFi.softAPIP().toString();
    }
  }
  if (var == "HOSTNAME") {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char hostname[20];
    sprintf(hostname, "esp32-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(hostname);
  }
  if (var == "SSID") {
    return currentConfig.wifi_ssid;
  }
  if (var == "MQTT_STATUS") {
    if (currentConfig.board_id == 0) {
      return "Unconfigured";
    }
    return client.connected() ? "Connected" : "Disconnected";
  }
  if (var == "UPTIME") {
    unsigned long seconds = millis() / 1000;
    unsigned long days = seconds / 86400;
    seconds %= 86400;
    unsigned long hours = seconds / 3600;
    seconds %= 3600;
    unsigned long minutes = seconds / 60;
    seconds %= 60;

    char uptime[32];
    if (days > 0) {
      sprintf(uptime, "%lud %02luh %02lum", days, hours, minutes);
    } else if (hours > 0) {
      sprintf(uptime, "%luh %02lum %02lus", hours, minutes, seconds);
    } else {
      sprintf(uptime, "%lum %02lus", minutes, seconds);
    }
    return String(uptime);
  }
  if (var == "RSSI") {
    if (WiFi.status() == WL_CONNECTED) {
      return String(WiFi.RSSI()) + " dBm";
    }
    return "N/A";
  }
  if (var == "FREE_MEMORY") {
    float freeKB = ESP.getFreeHeap() / 1024.0;
    char mem[16];
    sprintf(mem, "%.1f KB", freeKB);
    return String(mem);
  }
  if (var == "DATETIME") {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      return "Not synced";
    }
    char dateTimeStr[32];
    strftime(dateTimeStr, sizeof(dateTimeStr), "%m-%d-%y %H:%M:%S", &timeinfo);
    return String(dateTimeStr);
  }
  if (var == "CAN_STATUS") {
    // Placeholder for CAN bus status
    return "N/A";
  }
  if (var == "CAN_ID") {
    // Return board ID as CAN ID
    return String(currentConfig.board_id);
  }
  return String();
}

// State tracking
// None needed for base template

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.println("--------------------------------");
  Serial.println("Starting HSC-ESP32-Base");
  Serial.println("FW Rev: " + String(FW_VERSION));
  Serial.println("Board ID: " + String(currentConfig.board_id));
  Serial.println("--------------------------------");
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(currentConfig.wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(currentConfig.wifi_ssid.c_str(),
             currentConfig.wifi_password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi. Starting Fallback AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("HSC-Setup", "password");
    Serial.println("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Configure NTP
    Serial.println("Configuring NTP...");
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("NTP configured (will sync in background)");
  }
}

void reconnect() {
  if (WiFi.status() != WL_CONNECTED)
    return; // Don't try MQTT if no WiFi

  if (currentConfig.board_id == 0)
    return; // Don't connect if unconfigured

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "HSC-Device-";
    clientId += String(currentConfig.board_id);

    if (client.connect(clientId.c_str(), currentConfig.mqtt_user.c_str(),
                       currentConfig.mqtt_password.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement with device info
      String topic = "hsc/device/announce";
      String deviceName = "HSC-Device-" + String(currentConfig.board_id);

      // Get MAC address for hostname
      uint8_t mac[6];
      WiFi.macAddress(mac);
      char hostname[20];
      sprintf(hostname, "esp32-%02X%02X%02X", mac[3], mac[4], mac[5]);

      String ipAddress = WiFi.localIP().toString();
      String payload = deviceName + "," + String(hostname) + "," + ipAddress;
      client.publish(topic.c_str(), payload.c_str(), true); // Retained

      // Also publish board status
      String statusTopic =
          "hsc/device/status/" + String(currentConfig.board_id);
      client.publish(statusTopic.c_str(), "online");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize Pins
  pinMode(2, OUTPUT);   // Initialize LED pin
  digitalWrite(2, LOW); // Ensure LED is off initially

  // Initialize AP Mode Button
  pinMode(PIN_AP_BUTTON, INPUT_PULLUP);

  // Initialize Config
  if (!configManager.begin()) {
    Serial.println("Failed to initialize ConfigManager");
  }
  currentConfig = configManager.load();

  setup_wifi();
  client.setServer(currentConfig.mqtt_server.c_str(), currentConfig.mqtt_port);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load favicon.ico file
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/favicon.ico", "image/x-icon");
  });

  // API: Get Settings
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    StaticJsonDocument<512> doc;
    doc["wifi_ssid"] = currentConfig.wifi_ssid;
    doc["wifi_password"] = currentConfig.wifi_password;
    doc["mqtt_server"] = currentConfig.mqtt_server;
    doc["mqtt_port"] = currentConfig.mqtt_port;
    doc["mqtt_user"] = currentConfig.mqtt_user;
    doc["mqtt_password"] = currentConfig.mqtt_password;
    doc["board_id"] = currentConfig.board_id;
    doc["location"] = currentConfig.location;
    serializeJson(doc, *response);
    request->send(response);
  });

  // API: Save Settings
  server.on(
      "/api/settings", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // This handler is called after body is received
      },
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        static String body;
        if (index == 0)
          body = "";
        for (size_t i = 0; i < len; i++)
          body += (char)data[i];

        if (index + len == total) {
          StaticJsonDocument<512> doc;
          DeserializationError error = deserializeJson(doc, body);
          if (error) {
            request->send(
                400, "application/json",
                "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
          }

          Config newConfig;
          newConfig.wifi_ssid = doc["wifi_ssid"] | currentConfig.wifi_ssid;
          newConfig.wifi_password =
              doc["wifi_password"] | currentConfig.wifi_password;
          newConfig.mqtt_server =
              doc["mqtt_server"] | currentConfig.mqtt_server;
          newConfig.mqtt_port = doc["mqtt_port"] | currentConfig.mqtt_port;
          newConfig.mqtt_user = doc["mqtt_user"] | currentConfig.mqtt_user;
          newConfig.mqtt_password =
              doc["mqtt_password"] | currentConfig.mqtt_password;
          newConfig.board_id = doc["board_id"] | currentConfig.board_id;
          newConfig.location = doc["location"] | currentConfig.location;

          if (configManager.save(newConfig)) {
            currentConfig = newConfig; // Update in-memory
            request->send(200, "application/json",
                          "{\"status\":\"success\",\"message\":\"Settings "
                          "saved. Rebooting...\"}");
            delay(1000);
            ESP.restart();
          } else {
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"message\":\"Failed to save "
                          "settings\"}");
          }
        }
      });

  // API: Reset Settings
  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    configManager.reset();
    request->send(200, "application/json",
                  "{\"status\":\"success\",\"message\":\"Settings reset. "
                  "Rebooting...\"}");
    delay(1000);
    ESP.restart();
  });
  // API: Toggle Locate (Real-time, no persistence)
  server.on("/api/locate", HTTP_POST, [](AsyncWebServerRequest *request) {
    String state;
    bool fromPost = false;

    // Prefer POST param if present, else fall back to query
    if (request->hasParam("state", true)) { // POST body
      state = request->getParam("state", true)->value();
      fromPost = true;
    } else if (request->hasParam("state")) { // query string
      state = request->getParam("state")->value();
    } else {
      request->send(
          400, "application/json",
          "{\"status\":\"error\",\"message\":\"Missing state param\"}");
      return;
    }

    locateActive = (state == "true" || state == "1");
    Serial.printf("Locate toggled to: %s (from %s)\n",
                  locateActive ? "true" : "false", fromPost ? "POST" : "query");

    request->send(200, "application/json", "{\"status\":\"success\"}");
  });

  // API: Restart Device
  server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json",
                  "{\"status\":\"success\",\"message\":\"Rebooting...\"}");
    shouldReboot = true;
  });

  // API: Get Status (for live footer updates)
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    StaticJsonDocument<256> doc;

    // Uptime
    unsigned long seconds = millis() / 1000;
    unsigned long days = seconds / 86400;
    seconds %= 86400;
    unsigned long hours = seconds / 3600;
    seconds %= 3600;
    unsigned long minutes = seconds / 60;
    seconds %= 60;

    char uptime[32];
    if (days > 0) {
      sprintf(uptime, "%lud %02luh %02lum", days, hours, minutes);
    } else if (hours > 0) {
      sprintf(uptime, "%luh %02lum %02lus", hours, minutes, seconds);
    } else {
      sprintf(uptime, "%lum %02lus", minutes, seconds);
    }
    doc["uptime"] = uptime;

    // RSSI
    if (WiFi.status() == WL_CONNECTED) {
      char rssi[16];
      sprintf(rssi, "%d dBm", WiFi.RSSI());
      doc["rssi"] = rssi;
    } else {
      doc["rssi"] = "N/A";
    }

    // Free Memory
    float freeKB = ESP.getFreeHeap() / 1024.0;
    char mem[16];
    sprintf(mem, "%.1f KB", freeKB);
    doc["free_memory"] = mem;

    // Date/Time from NTP
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char dateTimeStr[32];
      strftime(dateTimeStr, sizeof(dateTimeStr), "%m-%d-%y %H:%M:%S",
               &timeinfo);
      doc["runtime"] = dateTimeStr;
    } else {
      doc["runtime"] = "Not synced";
    }

    serializeJson(doc, *response);
    request->send(response);
  });

  server.begin();
}

void loop() {
  // Handle Reboot
  if (shouldReboot) {
    delay(1000); // Give time for response to be sent
    ESP.restart();
  }

  // Handle AP Mode Button (Reset Board ID to 0)
  static unsigned long apButtonPressStart = 0;
  static bool apButtonActive = false;

  if (digitalRead(PIN_AP_BUTTON) == LOW) {
    if (!apButtonActive) {
      apButtonActive = true;
      apButtonPressStart = millis();
    } else {
      if (millis() - apButtonPressStart > 3000) {
        Serial.println("AP Mode Button Held - Resetting WiFi Password");
        currentConfig.wifi_password = "password";
        configManager.save(currentConfig);
        shouldReboot = true;
        apButtonActive = false; // Prevent multiple triggers
        // Optional: Blink LED fast to indicate reset?
        for (int k = 0; k < 10; k++) {
          digitalWrite(2, !digitalRead(2));
          delay(100);
        }
      }
    }
  } else {
    apButtonActive = false;
  }

  // Handle Locate Blinking
  if (locateActive) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 500) {
      lastBlink = millis();
      digitalWrite(2, !digitalRead(2));
      Serial.println("Blinking LED...");
    }
  } else {
    digitalWrite(2, LOW);
  }

  // Handle MQTT
  if (currentConfig.board_id != 0) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}
