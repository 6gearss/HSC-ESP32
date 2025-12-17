#include "HSC_Base.h"
#include "config.h"
#include <time.h>

// Embedded HTML and CSS
static const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HSC Device Status</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <header>
        <h1>HSC Base Configuration</h1>
        <span id="headerLocation" class="header-location"></span>
    </header>
    <main>
        <div class="card">
            <h2>Configuration</h2>
            <form id="configForm">
                <h3>WiFi Settings</h3>
                <div class="form-group">
                    <label for="wifi_ssid">SSID:</label>
                    <input type="text" id="wifi_ssid" name="wifi_ssid" required>
                </div>
                <div class="form-group">
                    <label for="wifi_password">Password:</label>
                    <input type="password" id="wifi_password" name="wifi_password">
                </div>
                <h3>MQTT Settings</h3>
                <div class="form-group">
                    <label for="mqtt_server">Server:</label>
                    <input type="text" id="mqtt_server" name="mqtt_server" required>
                </div>
                <div class="form-group">
                    <label for="mqtt_port">Port:</label>
                    <input type="number" id="mqtt_port" name="mqtt_port" required>
                </div>
                <div class="form-group">
                    <label for="mqtt_user">User:</label>
                    <input type="text" id="mqtt_user" name="mqtt_user">
                </div>
                <div class="form-group">
                    <label for="mqtt_password">Password:</label>
                    <input type="password" id="mqtt_password" name="mqtt_password">
                </div>
                <h3>Device Settings</h3>
                <div class="form-group">
                    <label>Board Type:</label>
                    <span style="flex: 2; font-size: 0.82rem; color: var(--text-color);">%BOARD_TYPE%</span>
                </div>
                <div class="form-group">
                    <label for="board_id">Board ID:</label>
                    <input type="number" id="board_id" name="board_id" required>
                </div>
                <h3>Location Settings</h3>
                <div class="form-group">
                    <label for="location">Location:</label>
                    <input type="text" id="location" name="location">
                </div>
                <div class="actions">
                    <a href="/" class="btn-link">Home</a>
                    <a href="/device" class="btn-link">Device</a>
                    <a href="/firmware" class="btn-link">Firmware</a>
                    <button type="button" id="restartBtn" class="btn-link">Restart</button>
                    <button type="button" id="locateLink" class="btn-link">Locate Board</button>
                    <button type="submit" class="btn-link">Save Settings</button>
                    <button type="button" id="resetBtn" class="btn-link">Reset Defaults</button>
                </div>
            </form>
        </div>
        <script>
            let locateState = false;
            fetch('/api/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('wifi_ssid').value = data.wifi_ssid || '';
                    document.getElementById('wifi_password').value = data.wifi_password || '';
                    document.getElementById('mqtt_server').value = data.mqtt_server || '';
                    document.getElementById('mqtt_port').value = data.mqtt_port || 1883;
                    document.getElementById('mqtt_user').value = data.mqtt_user || '';
                    document.getElementById('mqtt_password').value = data.mqtt_password || '';
                    document.getElementById('board_id').value = (data.board_id !== undefined) ? data.board_id : 1;
                    document.getElementById('location').value = data.location || '';
                    document.getElementById('headerLocation').textContent = data.location || '';
                    locateState = false;
                    document.getElementById('locateLink').textContent = 'Locate Board';
                })
                .catch(err => console.error('Failed to load settings:', err));
            document.getElementById('configForm').addEventListener('submit', function (e) {
                e.preventDefault();
                const formData = new FormData(this);
                const data = {};
                formData.forEach((value, key) => {
                    if (key === 'mqtt_port' || key === 'board_id') {
                        data[key] = parseInt(value);
                    } else {
                        data[key] = value;
                    }
                });
                fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data),
                })
                    .then(response => response.json())
                    .then(data => {
                        alert(data.message);
                        if (data.status === 'success') location.reload();
                    })
                    .catch((error) => {
                        console.error('Error:', error);
                        alert('Failed to save settings');
                    });
            });
            document.getElementById('restartBtn').addEventListener('click', function () {
                if (confirm('Are you sure you want to restart the device?')) {
                    fetch('/api/restart', { method: 'POST' })
                        .then(response => response.json())
                        .then(data => {
                            alert(data.message);
                            setTimeout(() => location.reload(), 5000);
                        })
                        .catch(err => alert('Failed to restart'));
                }
            });

            document.getElementById('resetBtn').addEventListener('click', function () {
                if (confirm('Are you sure you want to reset all settings to defaults? This will reboot the device.')) {
                    fetch('/api/reset', { method: 'POST' })
                        .then(response => response.json())
                        .then(data => {
                            alert(data.message);
                            location.reload();
                        })
                        .catch(err => alert('Failed to reset settings'));
                }
            });
            document.getElementById('locateLink').addEventListener('click', function () {
                locateState = !locateState;
                fetch('/api/locate?state=' + locateState, { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.status === 'success') {
                            document.getElementById('locateLink').textContent =
                                locateState ? 'Stop Locate' : 'Locate Board';
                        } else {
                            console.error('Failed to toggle locate');
                            locateState = !locateState;
                        }
                    })
                    .catch(err => {
                        console.error('Error toggling locate:', err);
                        locateState = !locateState;
                    });
            });
        </script>
    </main>
    <footer>
        <div class="footer-grid">
            <div class="footer-row">
                <div class="footer-pair">
                    <span class="label">Host:</span>
                    <span class="value">%HOSTNAME%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">IP:</span>
                    <span class="value">%IP%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">SSID:</span>
                    <span class="value">%SSID%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">Signal:</span>
                    <span class="value" id="rssi">%RSSI%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">MQTT:</span>
                    <span class="value">%MQTT_STATUS%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">CAN ID:</span>
                    <span class="value">%CAN_ID%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">CAN:</span>
                    <span class="value">%CAN_STATUS%</span>
                </div>
            </div>
            <div class="footer-row">
                <div class="footer-pair">
                    <span class="label">FW:</span>
                    <span class="value">%FW_REV%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">Date/Time:</span>
                    <span class="value" id="runtime">%DATETIME%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">Uptime:</span>
                    <span class="value" id="uptime">%UPTIME%</span>
                </div>
                <div class="footer-pair">
                    <span class="label">Free Mem:</span>
                    <span class="value" id="freemem">%FREE_MEMORY%</span>
                </div>
            </div>
        </div>
        <div class="copyright">
            HSC Engineering &copy;2025
        </div>
    </footer>
    <script>
        setInterval(() => {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    if (data.uptime) document.getElementById('uptime').textContent = data.uptime;
                    if (data.rssi) document.getElementById('rssi').textContent = data.rssi;
                    if (data.free_memory) document.getElementById('freemem').textContent = data.free_memory;
                    if (data.runtime) document.getElementById('runtime').textContent = data.runtime;
                })
                .catch(err => console.error('Failed to refresh status:', err));
        }, 2000);
    </script>
</body>
</html>
)rawliteral";

static const char style_css[] PROGMEM = R"rawliteral(
:root {
    --primary-color: #2563eb;
    --secondary-color: #f3f4f6;
    --text-color: #111827;
    --muted-text: #6b7280;
    --bg-color: #f9fafb;
    --card-bg: #ffffff;
    --border-color: #e5e7eb;
    --accent-border: #d1d5db;
    --footer-bg: #0f172a;
    --footer-text: #e5e7eb;
}
*, *::before, *::after { box-sizing: border-box; }
body {
    font-family: system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    background: var(--bg-color);
    color: var(--text-color);
    margin: 0;
    padding: 0;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    font-size: 13px;
    line-height: 1.4;
}
header {
    background: #ffffff;
    border-bottom: 1px solid var(--border-color);
    padding: 8px 20px;
    display: flex;
    align-items: center;
    justify-content: space-between;
}
h1 {
    margin: 0;
    font-size: 1.1rem;
    font-weight: 600;
    letter-spacing: 0.01em;
}
.header-location {
    font-size: 0.9rem;
    color: var(--muted-text);
    font-weight: 500;
}
main {
    flex: 1;
    padding: 16px;
    max-width: 1100px;
    margin: 0 auto;
    width: 100%;
}
.card {
    background: var(--card-bg);
    border-radius: 6px;
    padding: 14px 16px;
    margin-bottom: 12px;
    border: 1px solid var(--border-color);
}
.card h2 {
    margin: 0 0 10px 0;
    font-size: 0.98rem;
    font-weight: 600;
    color: var(--text-color);
    letter-spacing: 0.01em;
}
h3 {
    font-size: 0.9rem;
    color: var(--muted-text);
    margin: 14px 0 8px;
    font-weight: 500;
}
.form-group {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 8px;
}
label {
    flex: 1;
    font-size: 0.82rem;
    font-weight: 500;
    color: var(--muted-text);
}
input {
    flex: 2;
    padding: 5px 8px;
    font-size: 0.82rem;
    border-radius: 5px;
    border: 1px solid var(--accent-border);
    background: #f9fafb;
    outline: none;
    transition: border-color 0.15s ease, background-color 0.15s ease, box-shadow 0.15s ease;
}
input:focus {
    border-color: var(--primary-color);
    background-color: #ffffff;
    box-shadow: 0 0 0 1px rgba(37, 99, 235, 0.2);
}
.actions {
    margin-top: 2rem;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
}
.btn-link {
    background: none;
    border: none;
    color: var(--primary-color);
    padding: 0;
    font-size: 0.82rem;
    cursor: pointer;
    text-decoration: underline;
}
.btn-link:hover {
    color: #1d4ed8;
}
footer {
    background: var(--footer-bg);
    color: var(--footer-text);
    padding: 8px 20px;
    font-size: 0.75rem;
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    border-top: 1px solid #1f2937;
}
.footer-grid {
    display: flex;
    flex-direction: column;
    gap: 6px;
    flex: 1;
}
.footer-row {
    display: flex;
    gap: 20px;
    flex-wrap: wrap;
}
.footer-pair {
    display: flex;
    gap: 8px;
    align-items: baseline;
}
footer .label {
    font-weight: 600;
    color: #cbd5e1;
    white-space: nowrap;
}
footer .value {
    font-family: ui-monospace, Menlo, Consolas, monospace;
    color: #f8fafc;
    white-space: nowrap;
}
.copyright {
    white-space: nowrap;
    margin-left: 24px;
    color: #94a3b8;
    font-size: 0.65rem;
    align-self: center;
}
@media (max-width: 640px) {
    main { padding: 12px; }
    .form-group {
        flex-direction: column;
        align-items: stretch;
    }
    label { margin-bottom: 2px; }
    .footer-grid { gap: 8px; }
    .footer-row { gap: 12px; }
}
)rawliteral";

HSC_Base::HSC_Base() : server(80), mqttClient(espClient) {
  boardTypeDesc = BOARD_TYPE_DESC;
  boardTypeShort = BOARD_TYPE_SHORT;
}

#include <HTTPClient.h>

void HSC_Base::setUpdateUrl(const char *url) { _preConfigUpdateUrl = url; }

void HSC_Base::setBoardInfo(const char *desc, const char *shortName,
                            const char *fwVersion) {
  boardTypeDesc = desc;
  boardTypeShort = shortName;
  firmwareVersion = fwVersion;
}

void HSC_Base::begin() {
  Serial.begin(115200);

  // Initialize LED
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  // Initialize AP Mode Button
  pinMode(PIN_AP_BUTTON, INPUT_PULLUP);

  // Initialize Config
  if (!configManager.begin()) {
    Serial.println("Failed to initialize ConfigManager");
  }
  currentConfig = configManager.load();

  // Apply update URL from setup() if available
  if (_preConfigUpdateUrl.length() > 0) {
    currentConfig.update_url = _preConfigUpdateUrl;
  }

  setupWifi();
  mqttClient.setServer(currentConfig.mqtt_server.c_str(),
                       currentConfig.mqtt_port);

  setupWebServer();
  server.begin();

  // Initialize Identity
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macBuf[20];
  sprintf(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
  macStr = String(macBuf);

  char devIdBuf[32];
  String shortName = String(boardTypeShort);
  shortName.toLowerCase();
  sprintf(devIdBuf, "%s-%02x%02x%02x", shortName.c_str(), mac[3], mac[4],
          mac[5]);
  deviceId = String(devIdBuf);

  // Approximate boot time (will be refined when NTP syncs)
  bootTime = time(nullptr);
}

void HSC_Base::loop() {
  // Handle Reboot
  if (shouldReboot) {
    delay(1000);
    ESP.restart();
  }

  // Handle AP Mode Button
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
        apButtonActive = false;
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
    }
  } else {
    digitalWrite(2, LOW);
  }

  // Handle Update
  if (shouldUpdate) {
    shouldUpdate = false;
    performOTA(currentConfig.update_url);
  }

  // Handle MQTT
  if (currentConfig.board_id != 0) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = now;
        reconnectMqtt();
      }
    }
    mqttClient.loop();
  }
}

void HSC_Base::setupWifi() {
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

  // Set Hostname
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char hostname[32];
  String shortName = String(boardTypeShort);
  shortName.toLowerCase();
  sprintf(hostname, "%s-%02x%02x%02x", shortName.c_str(), mac[3], mac[4],
          mac[5]);
  WiFi.setHostname(hostname);
  Serial.print("Hostname: ");
  Serial.println(hostname);

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

    Serial.println("Configuring NTP...");
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("NTP configured (will sync in background)");
  }
}

void HSC_Base::reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (currentConfig.board_id == 0)
    return;

  Serial.print("Attempting MQTT connection...");
  String clientId = "HSC-Device-";
  clientId += String(currentConfig.board_id);

  if (mqttClient.connect(deviceId.c_str(), currentConfig.mqtt_user.c_str(),
                         currentConfig.mqtt_password.c_str(),
                         ("HSC/devices/" + deviceId + "/status").c_str(), 0,
                         true, "offline")) {
    Serial.println("connected");

    // 1. Publish Online Status (Retained)
    String statusTopic = "HSC/devices/" + deviceId + "/status";
    mqttClient.publish(statusTopic.c_str(), "online", true);

    // 2. Publish Device Information (Retained)
    // Calculate boot time based on current time - uptime
    time_t now;
    time(&now);
    time_t actualBootTime = now - (millis() / 1000);

    StaticJsonDocument<512> doc;
    doc["hostname"] = deviceId;
    doc["model"] = boardTypeDesc;
    doc["board_code"] = boardTypeShort;
    doc["firmware"] = firmwareVersion;
    doc["mac"] = macStr;
    doc["ip"] = WiFi.localIP().toString();
    doc["boot_time"] = actualBootTime;

    String infoTopic = "HSC/devices/" + deviceId + "/info";
    char buffer[512];
    serializeJson(doc, buffer);
    mqttClient.publish(infoTopic.c_str(), buffer, true);

    // 3. Optional Boot Announcement (Non-retained)
    // We send this every time we reconnect, which acts as a "device allows" or
    // "hello" message
    StaticJsonDocument<128> bootDoc;
    bootDoc["hostname"] = deviceId;
    bootDoc["event"] = "boot"; // or 'reconnect' if we wanted to be specific
    char bootBuf[128];
    serializeJson(bootDoc, bootBuf);
    mqttClient.publish("HSC/devices/announce", bootBuf, false);

    // 4. Subscribe to Configuration
    String configTopic = "HSC/devices/" + deviceId + "/config";
    mqttClient.subscribe(configTopic.c_str());
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
  }
}

String HSC_Base::processor(const String &var) {
  if (var == "FW_REV") {
    return firmwareVersion;
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
    char hostname[32];
    String shortName = String(boardTypeShort);
    shortName.toLowerCase();
    sprintf(hostname, "%s-%02x%02x%02x", shortName.c_str(), mac[3], mac[4],
            mac[5]);
    return String(hostname);
  }
  if (var == "SSID") {
    return currentConfig.wifi_ssid;
  }
  if (var == "MQTT_STATUS") {
    if (currentConfig.board_id == 0) {
      return "Unconfigured";
    }
    return mqttClient.connected() ? "Connected" : "Disconnected";
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
    return "N/A";
  }
  if (var == "CAN_ID") {
    return String(currentConfig.board_id);
  }
  if (var == "BOARD_TYPE") {
    return boardTypeDesc;
  }
  if (var == "BOARD_TYPE_SHORT") {
    return boardTypeShort;
  }
  return String();
}

void HSC_Base::setupWebServer() {
  // Serve embedded index.html
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html,
                    [this](const String &var) { return processor(var); });
  });

  // Serve embedded style.css
  server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send_P(200, "text/css", style_css);
  });

  // Serve device.html from SPIFFS
  server.on("/device", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/device.html")) {
      request->send(SPIFFS, "/device.html", "text/html", false,
                    [this](const String &var) { return processor(var); });
    } else {
      request->send(404, "text/plain", "Device page not found");
    }
  });

  // Serve firmware.html from SPIFFS
  server.on("/firmware", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/firmware.html")) {
      request->send(SPIFFS, "/firmware.html", "text/html", false,
                    [this](const String &var) { return processor(var); });
    } else {
      request->send(404, "text/plain", "Firmware page not found");
    }
  });

  // Serve favicon.ico from SPIFFS
  server.on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (SPIFFS.exists("/favicon.ico")) {
      request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    } else {
      request->send(404, "text/plain", "Favicon not found");
    }
  });

  // API: Get Settings
  server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
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
      "/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
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
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"message\":\"Invalid "
                          "JSON\"}");
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
            currentConfig = newConfig;
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
  server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    configManager.reset();
    request->send(200, "application/json",
                  "{\"status\":\"success\",\"message\":\"Settings reset. "
                  "Rebooting...\"}");
    delay(1000);
    ESP.restart();
  });

  // API: Toggle Locate
  server.on("/api/locate", HTTP_POST, [this](AsyncWebServerRequest *request) {
    String state;
    if (request->hasParam("state", true)) {
      state = request->getParam("state", true)->value();
    } else if (request->hasParam("state")) {
      state = request->getParam("state")->value();
    } else {
      request->send(400, "application/json",
                    "{\"status\":\"error\",\"message\":\"Missing state "
                    "param\"}");
      return;
    }

    locateActive = (state == "true" || state == "1");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  });

  // API: Restart Device
  server.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json",
                  "{\"status\":\"success\",\"message\":\"Rebooting...\"}");
    shouldReboot = true;
  });

  // API: OTA Update
  server.on("/api/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json",
                  "{\"status\":\"success\",\"message\":\"Update started. Check "
                  "Serial Monitor. Device will reboot...\"}");
    shouldUpdate = true;
  });

  // API: Check Firmware
  server.on(
      "/api/firmware/check", HTTP_GET, [this](AsyncWebServerRequest *request) {
        const char *currentVersion = firmwareVersion.c_str();
        String updateUrl = currentConfig.update_url;
        if (updateUrl.length() == 0) {
          request->send(400, "application/json",
                        "{\"status\":\"error\",\"message\":\"No update URL "
                        "configured\"}");
          return;
        }

        // Resolve URL
        updateUrl.replace("%BOARD_TYPE%", boardTypeShort);

        // Derive Metadata URL (replace extension .bin with .json)
        String checkUrl = updateUrl;
        int dotIndex = checkUrl.lastIndexOf('.');
        if (dotIndex != -1) {
          checkUrl = checkUrl.substring(0, dotIndex) + ".json";
        } else {
          checkUrl += ".json";
        }

        WiFiClient client;
        HTTPClient http;
        http.begin(client, checkUrl);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          StaticJsonDocument<1024> remoteDoc;
          DeserializationError error = deserializeJson(remoteDoc, payload);

          if (!error) {
            String remoteVersion = remoteDoc["version"] | "unknown";
            String notes = remoteDoc["notes"] | "";

            bool updateAvailable = remoteVersion != String(currentVersion);

            // Construct response
            StaticJsonDocument<1024> resDoc;
            resDoc["current_version"] = currentVersion;
            resDoc["remote_version"] = remoteVersion;
            resDoc["update_available"] = updateAvailable;
            resDoc["notes"] = notes;

            String resStr;
            serializeJson(resDoc, resStr);
            request->send(200, "application/json", resStr);
          } else {
            request->send(
                502, "application/json",
                "{\"status\":\"error\",\"message\":\"Invalid JSON from "
                "server\"}");
          }
        } else {
          request->send(
              502, "application/json",
              "{\"status\":\"error\",\"message\":\"Failed to fetch update "
              "metadata\"}");
        }
        http.end();
      });

  // API: Get Status
  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");
    StaticJsonDocument<256> doc;

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

    if (WiFi.status() == WL_CONNECTED) {
      char rssi[16];
      sprintf(rssi, "%d dBm", WiFi.RSSI());
      doc["rssi"] = rssi;
    } else {
      doc["rssi"] = "N/A";
    }

    float freeKB = ESP.getFreeHeap() / 1024.0;
    char mem[16];
    sprintf(mem, "%.1f KB", freeKB);
    doc["free_memory"] = mem;

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
}

void HSC_Base::registerPage(const char *uri, ArRequestHandlerFunction handler) {
  server.on(uri, HTTP_GET, handler);
}

void HSC_Base::registerApi(const char *uri, WebRequestMethodComposite method,
                           ArRequestHandlerFunction handler) {
  server.on(uri, method, handler);
}

void HSC_Base::performOTA(const String &url) {
  if (url.length() == 0) {
    Serial.println("OTA Error: No URL configured");
    return;
  }

  String finalUrl = url;
  finalUrl.replace("%BOARD_TYPE%", boardTypeShort);

  // Check metadata for SPIFFS update
  String checkUrl = finalUrl;
  int dotIndex = checkUrl.lastIndexOf('.');
  if (dotIndex != -1) {
    checkUrl = checkUrl.substring(0, dotIndex) + ".json";
  } else {
    checkUrl += ".json";
  }

  bool updateSpiffs = false;
  WiFiClient client;
  HTTPClient http;

  // Reuse client logic for http/https in check
  if (checkUrl.startsWith("https")) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    http.begin(secureClient, checkUrl);
  } else {
    http.begin(client, checkUrl);
  }

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      updateSpiffs = doc["update_spiffs"] | false;
    }
  }
  http.end();

  if (updateSpiffs) {
    Serial.println("Filesystem update requested...");
    String spiffsUrl = finalUrl;
    if (dotIndex != -1) {
      spiffsUrl = spiffsUrl.substring(0, dotIndex) + ".spiffs.bin";
    } else {
      spiffsUrl += ".spiffs.bin";
    }
    Serial.println("SPIFFS URL: " + spiffsUrl);

    // Unmount SPIFFS to ensure safe update
    SPIFFS.end();

    httpUpdate.rebootOnUpdate(false); // Don't reboot after SPIFFS

    t_httpUpdate_return ret;
    if (spiffsUrl.startsWith("https")) {
      WiFiClientSecure secureClient;
      secureClient.setInsecure();
      ret = httpUpdate.updateSpiffs(secureClient, spiffsUrl);
    } else {
      ret = httpUpdate.updateSpiffs(client, spiffsUrl);
    }

    if (ret == HTTP_UPDATE_OK) {
      Serial.println("SPIFFS Update OK");
    } else {
      Serial.printf("SPIFFS Update Failed (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      // Try to recover SPIFFS mount if update failed
      SPIFFS.begin(true);
    }
  }

  Serial.println("Starting Firmware Update...");
  Serial.println("URL: " + finalUrl);

  httpUpdate.rebootOnUpdate(true); // Reboot after firmware

  if (finalUrl.startsWith("https")) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // Skip cert validation
    t_httpUpdate_return ret = httpUpdate.update(secureClient, finalUrl);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
    }
  } else {
    t_httpUpdate_return ret = httpUpdate.update(client, finalUrl);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
    }
  }
}
