#include "config.h"
#include <HSC_Base.h>
#include <SPIFFS.h>

HSC_Base hscBase;

void setup() {
  // Initialize SPIFFS for device.html
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialize the HSC_Base library
  hscBase.setBoardInfo(BOARD_TYPE_DESC, BOARD_TYPE_SHORT);
  hscBase.begin();

  // Register device-specific page (optional)
  hscBase.registerPage("/device", [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/device.html", String(), false,
                  [](const String &var) {
                    // Use the library's processor for all standard variables
                    return hscBase.processTemplate(var);
                  });
  });

  // Add any device-specific initialization here
}

void loop() {
  // Run the HSC_Base loop
  hscBase.loop();

  // Add any device-specific loop code here
}
