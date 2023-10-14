# Battery-Powered WiFi Clock with Environmental Monitoring

## Overview

This project is a battery-powered WiFi clock that not only displays the current time but also monitors the ambient temperature and light levels. The device is built on a Lilygo T7 board and uses an SSD1322 display, a MAX44009 light sensor, and a DS18B20 temperature sensor.

## Features

- Displays current time synced via NTP
- Monitors ambient temperature using DS18B20 sensor
- Measures ambient light levels using MAX44009 sensor
- Adjusts display brightness based on ambient light
- Battery level indicator
- WiFi connectivity for time synchronization
- Automatic reconnection attempts for WiFi
- Battery-powered with a single LiPo cell
- Battery level monitoring and display

## Hardware Requirements

- Lilygo T7 board
- SSD1322 OLED display
- MAX44009 light sensor
- DS18B20 temperature sensor
- Single LiPo cell
- Miscellaneous: Wires, breadboard, etc.

## Software Requirements

- Arduino IDE
- Required Libraries:
  - `Arduino.h`
  - `NTPClient.h`
  - `WiFi.h`
  - `WiFiUdp.h`
  - `U8g2lib.h`
  - `SPI.h`
  - `TimeLib.h`
  - `OneWire.h`
  - `DallasTemperature.h`
  - `Wire.h`
  - `Max44009.h`
  - `Battery18650Stats.h`

## Installation

1. Clone this repository or download the code.
2. Open the Arduino IDE and load the code.
3. Install the required libraries via the Arduino Library Manager.
4. Connect your Lilygo T7 board to your computer.
5. Select the correct board and port in the Arduino IDE.
6. Compile and upload the code to the board.

## Configuration

- Update the `owm_credentials.h` file with your WiFi credentials.
- Set the NTP server and time offset according to your location.
- Adjust the lux thresholds and contrast levels as needed.

## Usage

Once the code is uploaded and the device is powered on, it will automatically connect to the WiFi network and sync the time via NTP. The display will show the current time, ambient temperature, and battery level. The display brightness will adjust automatically based on the ambient light level.

## Troubleshooting

- If the device fails to connect to WiFi, it will retry after a specified interval (default is 6 hours).
- If the NTP time fails to update, check your network connection and NTP server status.

## Contributing

Feel free to fork this repository and submit pull requests for any enhancements or bug fixes.

## License

This project is open-source and available under the MIT License.

