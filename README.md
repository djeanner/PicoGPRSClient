# PicoGPRSClient
This Arduino sketch is designed to test and debug GPRS-based HTTP POST requests using the AIR780 module (via AT commands) on platforms like the Raspberry Pi Pico (or compatible boards).

main ino file: src/main.cpp
## Features

    ✅ Verbose debug mode (verbose = true) for rich serial logging

    🔁 Optional LED blink diagnostics to indicate status (success/failure)

    🌐 Sends POST request to httpbin.org with sample payload

    🔧 Supports manual passthrough mode via USB serial

    📡 GPRS configuration with APN setup and IP stack initialization

    🔍 Includes basic diagnostic AT commands for signal, registration, operator, etc.

## Configuration

    verbose: Set to false to reduce serial output

    testHttpBin: Set to true to test response parsing and device echo from httpbin

    passthroughEnabled: When true, allows manual command interaction with AIR780 over USB

## Hardware Requirements

    Raspberry Pi Pico (or similar with UART)
    AIR780 GSM/GPRS module
    USB-to-Serial interface (for logging and manual input)
    Optional: onboard LED (e.g., GPIO25 on Pico)

