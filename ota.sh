#!/bin/bash

# Extract BOARD_TYPE_SHORT from src/config.h
# Handling multi-line declaration:
# static const char BOARD_TYPE_SHORT[] =
#     "NEW";
BOARD_NAME=$(grep -A 1 'static const char BOARD_TYPE_SHORT\[\] =' src/config.h | tail -n 1 | awk -F'"' '{print $2}')

if [ -z "$BOARD_NAME" ]; then
    echo "Error: Could not extract BOARD_TYPE_SHORT from src/config.h"
    exit 1
fi

echo "Board Name detected: $BOARD_NAME"

# Extract FW_VERSION from src/config.h
# static const char FW_VERSION[] = "0.2.0";
VERSION=$(grep 'static const char FW_VERSION\[\] =' src/config.h | awk -F'"' '{print $2}')

if [ -z "$VERSION" ]; then
    echo "Warning: Could not extract FW_VERSION from src/config.h, defaulting to 0.0.0"
    VERSION="0.0.0"
fi
echo "Firmware Version: $VERSION"

# Run PlatformIO commands
echo "Running: pio run --target buildfs"
pio run --target buildfs
if [ $? -ne 0 ]; then
    echo "Error: pio run --target buildfs failed"
    exit 1
fi

echo "Running: pio run (Build Firmware)"
pio run
if [ $? -ne 0 ]; then
    echo "Error: pio run failed"
    exit 1
fi

# Define source directory for artifacts (assuming nodemcu-32s based on platformio.ini)
# If multiple environments exist, this might need adjustment, but usually there's a default or specific one used.
BUILD_DIR=".pio/build/nodemcu-32s"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory $BUILD_DIR does not exist."
    exit 1
fi

# Define destination filenames
FW_NAME="firmware_${BOARD_NAME}.bin"
FS_NAME="firmware_${BOARD_NAME}.spiffs.bin"

# Copy files
if [ -f "$BUILD_DIR/firmware.bin" ]; then
    echo "Copying $BUILD_DIR/firmware.bin to ./$FW_NAME"
    cp "$BUILD_DIR/firmware.bin" "./$FW_NAME"
else
    echo "Warning: firmware.bin not found in $BUILD_DIR"
fi

# Handle filesystem binary (spiffs.bin or littlefs.bin)
if [ -f "$BUILD_DIR/spiffs.bin" ]; then
    echo "Copying $BUILD_DIR/spiffs.bin to ./$FS_NAME"
    cp "$BUILD_DIR/spiffs.bin" "./$FS_NAME"
elif [ -f "$BUILD_DIR/littlefs.bin" ]; then
    # If using LittleFS, we might want to name it differently or keep spiffs_ prefix if that's the convention.
    # We'll stick to the requested structure which implies a filesystem image.
    echo "Copying $BUILD_DIR/littlefs.bin to ./$FS_NAME"
    cp "$BUILD_DIR/littlefs.bin" "./$FS_NAME"
else
    echo "Warning: Filesystem binary (spiffs.bin/littlefs.bin) not found in $BUILD_DIR"
fi

echo "OTA Artifacts prepared:"
# Create JSON manifest
JSON_NAME="firmware_${BOARD_NAME}.json"
CURRENT_DATE=$(date "+%Y-%m-%d %H:%M:%S")

cat <<EOF > "$JSON_NAME"
{
  "version": "$VERSION",
  "notes": "Build created on $CURRENT_DATE"
}
EOF

ls -l "$FW_NAME" "$FS_NAME" "$JSON_NAME" 2>/dev/null
