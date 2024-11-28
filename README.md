# CustomCarGaugesPrado
Custom Prado 150 series Gauges using 8 Oled 128x64 pixel displays, NodeMCU ESP32, TCA9548A I2C multiplexer and bluetooth elm327 obd reader. Started this project due to the fact that my Prado does not display Automatic Transmission temperatures, Transmission Lockup, and gear selection, from the factory. This Unit therefore requests this data from the ECU and displays the values as gauge graphics.

This project is designed to connect an ESP32 to an OBD-II vehicle interface using Bluetooth to retrieve vehicle telemetry data. The data is displayed on multiple SH1106 OLED screens via I2C using a TCA9548A I2C multiplexer. 

## Features

- **Bluetooth OBD-II Communication**: Uses `ELMduino` library to interact with the ELM327 OBD-II interface.
- **Multiple OLED Displays**: Displays connected via TCA9548A I2C multiplexer for simultaneous multi-display functionality.
- **Dynamic Display Mode**: Option to switch between normal and mirrored display modes using a button.
- **Vehicle Data Monitoring**: Retrieves and displays data such as transmission oil temperature, torque converter temperature, gear, coolant temperature, voltage, and torque converter lockup status.

## Hardware Requirements

- ESP32 Dev Module
- ELM327 OBD-II Bluetooth Module (or STN1170)
- SH1106 128x64 OLED Displays (multiple)
- TCA9548A I2C Multiplexer
- Button connected to pin `32`
- I2C lines connected to ESP32 pins `SCL (22)` and `SDA (21)`

## Software Requirements

- Arduino IDE
- `BluetoothSerial` library
- `ELMduino` library
- `U8g2` library for OLED displays
- `Wire` library for I2C communication

## Wiring Configuration

| Component         | ESP32 Pin |
|-------------------|------------|
| TCA9548A SCL      | 22         |
| TCA9548A SDA      | 21         |
| Button            | 32         |

## Functions Overview

- **`setup()`**: Initializes Bluetooth, I2C communication, and connects to the ELM327 module.
- **`loop()`**: Continuously reads OBD-II data and updates the displays.
- **`selectTCAChannel()`**: Selects the I2C channel on the multiplexer.
- **`displayMessage()`**: Displays a message on the specified OLED screen.
- **`drawTempGauge()`**: Draws a temperature gauge on the OLED display.
- **`displayVoltage()`**: Displays the current voltage from the OBD-II system.
- **`displayGear()`**: Displays the current gear information.
- **`displayTCL()`**: Displays the torque converter lockup status.
- **`loadTransTemps()`**: Retrieves transmission oil and torque converter temperatures.
- **`loadCurGearTCLockup()`**: Retrieves current gear and lockup status.

## Usage

1. **Setup**: Upload the sketch to the ESP32 using Arduino IDE.
2. **Bluetooth Pairing**: Ensure the ESP32 pairs with the OBD-II Bluetooth module (`Pin: 1234`).
3. **Data Monitoring**: Connect the ESP32 to your car’s OBD-II port and start the car to view live data.

## Future Improvements

- Add more OBD-II PIDs for advanced telemetry.
- Enhance UI with custom graphics for better data visualization.
- Implement data logging to an SD card or external storage.

Enjoy real-time vehicle telemetry with this customizable ESP32 and OLED display setup!
