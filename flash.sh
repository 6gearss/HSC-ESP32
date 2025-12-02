#!/bin/bash

set -e

ERASE=false
PORT_OPTION=""
POSITIONAL=()

# --- Helper: Show usage ---
show_help() {
    cat << EOF
Usage: $0 [OPTIONS] [BRANCH]

Options:
  -e, --erase           Erase flash before uploading
  -p, --port PORT       Specify port(s) to flash:
                          1 or 2        - Flash to first or second detected port
                          all or *      - Flash to all detected ports sequentially
                          /dev/ttyUSB0  - Flash to specific port path
                        If not specified, will use default or prompt if multiple ports found
  -h, --help            Show this help message

Arguments:
  BRANCH                Git branch to flash (defaults to current branch)

Examples:
  $0                    # Flash current branch to default port
  $0 -p 1               # Flash to first detected port
  $0 -p all             # Flash to all detected ports
  $0 -p /dev/ttyUSB0    # Flash to specific port
  $0 -e main            # Erase and flash 'main' branch
EOF
    exit 0
}

# --- Parse command line options ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        -e|--erase)
            ERASE=true
            shift
            ;;
        -p|--port)
            PORT_OPTION="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            ;;
        -*)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
        *)
            POSITIONAL+=("$1")
            shift
            ;;
    esac
done

# Restore positional args
set -- "${POSITIONAL[@]}"

# --- Detect available serial ports ---
detect_ports() {
    local ports=()
    if [ -d "/dev" ]; then
        # Look for common ESP32 serial ports
        for port in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART*; do
            if [ -e "$port" ]; then
                ports+=("$port")
            fi
        done
    fi
    echo "${ports[@]}"
}

# --- Get ports to flash ---
get_flash_ports() {
    local available_ports=($(detect_ports))
    local flash_ports=()
    
    if [ ${#available_ports[@]} -eq 0 ]; then
        echo "ERROR: No serial ports detected!" >&2
        exit 1
    fi
    
    # If no port option specified
    if [ -z "$PORT_OPTION" ]; then
        if [ ${#available_ports[@]} -eq 1 ]; then
            flash_ports=("${available_ports[0]}")
        else
            echo "Multiple serial ports detected:"
            for i in "${!available_ports[@]}"; do
                echo "  $((i+1)). ${available_ports[$i]}"
            done
            echo -n "Select port (1-${#available_ports[@]}, 'all', or Ctrl+C to cancel): "
            read selection
            if [ "$selection" = "all" ] || [ "$selection" = "*" ]; then
                flash_ports=("${available_ports[@]}")
            elif [[ "$selection" =~ ^[0-9]+$ ]] && [ "$selection" -ge 1 ] && [ "$selection" -le ${#available_ports[@]} ]; then
                flash_ports=("${available_ports[$((selection-1))]}")
            else
                echo "Invalid selection!"
                exit 1
            fi
        fi
    # Handle port option
    elif [ "$PORT_OPTION" = "all" ] || [ "$PORT_OPTION" = "*" ]; then
        flash_ports=("${available_ports[@]}")
    elif [[ "$PORT_OPTION" =~ ^[0-9]+$ ]]; then
        local idx=$((PORT_OPTION - 1))
        if [ $idx -ge 0 ] && [ $idx -lt ${#available_ports[@]} ]; then
            flash_ports=("${available_ports[$idx]}")
        else
            echo "ERROR: Port number $PORT_OPTION out of range (1-${#available_ports[@]})" >&2
            exit 1
        fi
    elif [ -e "$PORT_OPTION" ]; then
        flash_ports=("$PORT_OPTION")
    else
        echo "ERROR: Port '$PORT_OPTION' not found!" >&2
        exit 1
    fi
    
    echo "${flash_ports[@]}"
}

# --- Flash to a specific port ---
flash_to_port() {
    local port="$1"
    echo ""
    echo "========================================="
    echo "Flashing to port: $port"
    echo "========================================="
    
    # Optional erase
    if [ "$ERASE" = true ]; then
        echo "Erasing flash on $port..."
        pio run --target erase --upload-port "$port"
    fi
    
    # Upload filesystem first if present
    if [ -d "data" ]; then
        echo "Uploading filesystem to $port..."
        pio run -t uploadfs --upload-port "$port"
    fi
    
    # Upload firmware
    echo "Uploading firmware to $port..."
    pio run -t upload --upload-port "$port"
    
    echo "✓ Completed flashing to $port"
}

# --- Branch logic ---
CURRENT=$(git rev-parse --abbrev-ref HEAD)

if [ -z "$1" ]; then
    TARGET="$CURRENT"
    RETURN_BACK=false
else
    TARGET="$1"
    RETURN_BACK=true
fi

echo "Current branch: $CURRENT"
echo "Target branch:  $TARGET"

# --- Switch branches if needed ---
if [ "$TARGET" != "$CURRENT" ]; then
    echo "Switching to $TARGET..."
    git checkout "$TARGET"
fi

# --- Get ports to flash ---
FLASH_PORTS=($(get_flash_ports))

echo ""
echo "Will flash to ${#FLASH_PORTS[@]} port(s): ${FLASH_PORTS[*]}"
echo ""

# --- Flash to each port ---
for port in "${FLASH_PORTS[@]}"; do
    flash_to_port "$port"
done

# --- Restore previous branch ---
if [ "$RETURN_BACK" = true ] && [ "$TARGET" != "$CURRENT" ]; then
    echo ""
    echo "Returning to $CURRENT..."
    git checkout "$CURRENT"
fi

echo ""
echo "========================================="
echo "✓ All operations complete!"
echo "========================================="
