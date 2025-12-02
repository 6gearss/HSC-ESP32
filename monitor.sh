#!/bin/bash

set -e

PORT_OPTION=""
BAUD_RATE="115200"
FILTER=""

# --- Helper: Show usage ---
show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
  -p, --port PORT       Specify port to monitor:
                          1 or 2        - Monitor first or second detected port
                          /dev/ttyUSB0  - Monitor specific port path
                        If not specified, will use default or prompt if multiple ports found
  -b, --baud RATE       Set baud rate (default: 115200)
  -f, --filter FILTER   Apply PlatformIO filter (e.g., esp32_exception_decoder, colorize)
  -h, --help            Show this help message

Examples:
  $0                    # Monitor default port
  $0 -p 1               # Monitor first detected port
  $0 -p 2               # Monitor second detected port
  $0 -p /dev/ttyUSB0    # Monitor specific port
  $0 -p 1 -b 9600       # Monitor first port at 9600 baud
  $0 -p 1 -f esp32_exception_decoder  # Monitor with exception decoder
EOF
    exit 0
}

# --- Parse command line options ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--port)
            PORT_OPTION="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD_RATE="$2"
            shift 2
            ;;
        -f|--filter)
            FILTER="$2"
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
            echo "Unexpected argument: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

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

# --- Get port to monitor ---
get_monitor_port() {
    local available_ports=($(detect_ports))
    local monitor_port=""
    
    if [ ${#available_ports[@]} -eq 0 ]; then
        echo "ERROR: No serial ports detected!" >&2
        exit 1
    fi
    
    # If no port option specified
    if [ -z "$PORT_OPTION" ]; then
        if [ ${#available_ports[@]} -eq 1 ]; then
            monitor_port="${available_ports[0]}"
        else
            echo "Multiple serial ports detected:"
            for i in "${!available_ports[@]}"; do
                echo "  $((i+1)). ${available_ports[$i]}"
            done
            echo -n "Select port (1-${#available_ports[@]}, or Ctrl+C to cancel): "
            read selection
            if [[ "$selection" =~ ^[0-9]+$ ]] && [ "$selection" -ge 1 ] && [ "$selection" -le ${#available_ports[@]} ]; then
                monitor_port="${available_ports[$((selection-1))]}"
            else
                echo "Invalid selection!"
                exit 1
            fi
        fi
    # Handle port option
    elif [[ "$PORT_OPTION" =~ ^[0-9]+$ ]]; then
        local idx=$((PORT_OPTION - 1))
        if [ $idx -ge 0 ] && [ $idx -lt ${#available_ports[@]} ]; then
            monitor_port="${available_ports[$idx]}"
        else
            echo "ERROR: Port number $PORT_OPTION out of range (1-${#available_ports[@]})" >&2
            exit 1
        fi
    elif [ -e "$PORT_OPTION" ]; then
        monitor_port="$PORT_OPTION"
    else
        echo "ERROR: Port '$PORT_OPTION' not found!" >&2
        exit 1
    fi
    
    echo "$monitor_port"
}

# --- Get the port to monitor ---
MONITOR_PORT=$(get_monitor_port)

echo "========================================="
echo "Monitoring port: $MONITOR_PORT"
echo "Baud rate: $BAUD_RATE"
if [ -n "$FILTER" ]; then
    echo "Filter: $FILTER"
fi
echo "========================================="
echo ""
echo "Press Ctrl+C to exit"
echo ""

# --- Build the PlatformIO command ---
PIO_CMD="pio device monitor --port $MONITOR_PORT --baud $BAUD_RATE"

if [ -n "$FILTER" ]; then
    PIO_CMD="$PIO_CMD --filter $FILTER"
fi

# --- Start monitoring ---
eval $PIO_CMD
