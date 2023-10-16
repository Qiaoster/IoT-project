#!/bin/bash

ESP32_IP="192.168.130.249"
ESP32_PORT=80  # Use the port you defined in your ESP32 Arduino code (e.g., 80)

# Check if an argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <0|1|2>"
    exit 1
fi

# Map the input value to the corresponding command
case $1 in
    0)
        COMMAND="SET_OVERRIDE_OFF"
	echo "off"
        ;;
    1)
        COMMAND="SET_OVERRIDE_OPEN"
	echo "open"
        ;;
    2)
        COMMAND="SET_OVERRIDE_CLOSE"
	echo "close"
        ;;
    *)
        echo "Invalid input. Use 0, 1, or 2."
        exit 1
        ;;
esac

# Use netcat (nc) to send the command to the ESP32
echo -e "$COMMAND\r" | nc -w1 -n $ESP32_IP $ESP32_PORT
