# HSC-ESP32-Base

A reusable PlatformIO library for ESP32-based devices on the HSC train layout. This library provides core functionality including WiFi connectivity, MQTT communication, web configuration interface, and OTA updates.

## Features

- 🌐 **WiFi Management**: Automatic connection with fallback AP mode
- 📡 **MQTT Integration**: Built-in MQTT client with auto-reconnect
- 🖥️ **Web Interface**: Embedded configuration UI with live status updates
- 🔄 **OTA Updates**: Over-the-air firmware updates
- 💾 **Configuration Management**: Persistent settings stored in NVS
- 📍 **Device Location**: Configurable location tracking
- 🔍 **Locate Feature**: LED blinking for physical device identification
- 🎨 **Modern UI**: Responsive, professional web interface

## Quick Start

### Using as a Library (Recommended)

1. **Create a new PlatformIO project**:
   ```bash
   mkdir my-hsc-device
   cd my-hsc-device
   pio project init --board nodemcu-32s
   ```

2. **Add HSC_Base library**:
   ```bash
   # Option 1: Git submodule (recommended)
   git submodule add https://github.com/6gearss/HSC-ESP32.git lib/HSC_Base
   
   # Option 2: Copy the library
   cp -r /path/to/HSC-ESP32/lib/HSC_Base lib/
   ```

3. **Create `src/main.cpp`**:
   ```cpp
   #include "config.h"
   #include <HSC_Base.h>
   #include <SPIFFS.h>

   HSC_Base hscBase;

   void setup() {
     if (!SPIFFS.begin(true)) {
       Serial.println("SPIFFS mount failed");
       return;
     }
     
     hscBase.begin();
     
     // Add your device-specific initialization here
   }

   void loop() {
     hscBase.loop();
     
     // Add your device-specific logic here
   }
   ```

4. **Create `src/config.h`** to customize your device:
   ```cpp
   #ifndef CONFIG_H
   #define CONFIG_H

   // Define your board type
   #undef BOARD_TYPE_DESC
   #undef BOARD_TYPE_SHORT
   static const char BOARD_TYPE_DESC[] = "Yard Track Detector";
   static const char BOARD_TYPE_SHORT[] = "YARD";

   #endif
   ```

5. **Build and upload**:
   ```bash
   pio run --target upload
   ```

### Using as a Template

Clone this repository and modify it directly:
```bash
git clone https://github.com/6gearss/HSC-ESP32.git my-device
cd my-device
# Modify src/main.cpp and src/config.h for your needs
pio run --target upload
```

## Configuration

### Board Type

Define your device type in `src/config.h`:

```cpp
#undef BOARD_TYPE_DESC
#undef BOARD_TYPE_SHORT
static const char BOARD_TYPE_DESC[] = "Signal Controller";  // Full name for web UI
static const char BOARD_TYPE_SHORT[] = "SIGNAL";            // Short name for MQTT
```

This will:
- Display "Signal Controller" in the web interface
- Use "SIGNAL-{ID}" in MQTT topics (e.g., `SIGNAL-1`, `SIGNAL-2`)

### Default Settings

Default WiFi and MQTT settings are in `lib/HSC_Base/src/config.h`. You can override them in your project's `src/config.h`.

### Runtime Configuration

Access the web interface at `http://[device-ip]/` to configure:
- WiFi credentials
- MQTT server settings
- Board ID
- Device location

## Web Interface

### Main Configuration Page (`/`)
- WiFi and MQTT settings
- Device configuration (Board ID, Location)
- System information in footer
- Actions: Restart, Locate, Save, Reset

### Device Page (`/device`)
- Template for device-specific information
- Customizable content
- Same footer and navigation

### API Endpoints

- `GET /api/settings` - Get current configuration
- `POST /api/settings` - Save configuration
- `POST /api/restart` - Restart device
- `POST /api/reset` - Reset to defaults
- `POST /api/locate?state=true` - Toggle locate LED
- `GET /api/status` - Get live status (uptime, RSSI, memory, etc.)

## MQTT Topics

### Published by Device

- `hsc/device/announce` - Device announcement on connect
  - Payload: `{BOARD_TYPE_SHORT}-{ID},{hostname},{IP}`
- `hsc/device/status/{ID}` - Device online status
  - Payload: `online`

### Custom Topics

Use the library's MQTT client for your device-specific topics:

```cpp
hscBase.getMqttClient().publish("hsc/yard/track/1", "OCCUPIED");
```

## Hardware

### Supported Boards
- ESP32-DevKit
- NodeMCU-32S
- Any ESP32 board with 4MB+ flash

### Pin Definitions

Default pins (override in your `src/config.h`):
- **GPIO 2**: Built-in LED (for locate feature)
- **GPIO 4**: AP Mode button (hold 3s to reset WiFi password)

## Development

### Project Structure

```
HSC-ESP32-Base/
├── lib/HSC_Base/          # Reusable library
│   └── src/
│       ├── HSC_Base.h     # Main library header
│       ├── HSC_Base.cpp   # Implementation (embedded web assets)
│       ├── ConfigManager.h
│       ├── ConfigManager.cpp
│       └── config.h       # Library defaults
├── src/
│   ├── main.cpp           # Example application
│   └── config.h           # Project-specific overrides
├── data/
│   └── device.html        # Device-specific page template
├── platformio.ini
├── MAINTENANCE.md         # Detailed maintenance guide
└── README.md
```

### Updating the Library

When you update the base library:

1. **Edit library files** in `lib/HSC_Base/src/`
2. **Compile and test**: `pio run`
3. **Commit changes**: `git commit -am "Update library"`
4. **Tag version**: `git tag v0.2.0 && git push --tags`

Child projects using git submodules will get updates:
```bash
cd lib/HSC_Base
git pull origin main
cd ../..
pio run --target upload
```

### Embedded Web Assets

The HTML and CSS are embedded in `lib/HSC_Base/src/HSC_Base.cpp` as raw string literals. This ensures child projects automatically get UI updates when they recompile.

To update the web interface:
1. Edit the `index_html` or `style_css` strings in `HSC_Base.cpp`
2. Recompile the library
3. Child projects get the updates on next build

## Examples

See the `src/main.cpp` for a minimal example. For more complex examples:

### Adding a Custom Page

```cpp
hscBase.registerPage("/status", [](AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/status.html", String(), false,
                [](const String &var) {
                  return hscBase.processTemplate(var);
                });
});
```

### Adding Custom API Endpoints

```cpp
hscBase.registerApi("/api/custom", HTTP_GET, 
  [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
```

### Accessing Configuration

```cpp
Config& config = hscBase.getConfig();
Serial.println("Board ID: " + String(config.board_id));
Serial.println("Location: " + config.location);
```

## Troubleshooting

### Device won't connect to WiFi
- Hold the AP button (GPIO 4) for 3 seconds to reset WiFi password to "password"
- Connect to "HSC-Setup" AP and reconfigure

### Web interface not loading
- Check that you're accessing the correct IP address
- Verify the device is connected to WiFi (check serial monitor)

### MQTT not connecting
- Ensure Board ID is set (not 0)
- Verify MQTT server is reachable
- Check MQTT credentials

### Updates not appearing in child project
- Ensure you're using git submodule or have copied the latest library
- Run `pio run --target clean` then `pio run`

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

MIT License - See LICENSE file for details

## Version History

### v0.1.0 (2025-12-02)
- Initial release
- WiFi and MQTT connectivity
- Web configuration interface
- Embedded web assets
- Board type identification
- Template processor for dynamic content

## Support

For issues and questions:
- Open an issue on GitHub
- Check MAINTENANCE.md for detailed documentation

---

**HSC Engineering © 2025**
