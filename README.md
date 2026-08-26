# Masjed-P104-Esp

ESP8266 firmware for controlling LED Matrix displays (P10/P4.75) in Mosques. This project provides an automated display for prayer times, clock, dates, and announcements with a web-based configuration interface.

## Features

- **Prayer Times:** Accurate calculation based on geolocation (Latitude/Longitude) and calculation methods (Egypt, Makkah, Karachi, etc.).
- **Display Infos:**
  - Real-time Clock (HH:MM:SS).
  - Gregorian and Hijri Dates.
  - Temperature display (requires DS3231).
  - Next Prayer countdown and Iqama times.
- **Web Interface:** Fully configurable via a captive portal or web interface.
  - Set Time/Date.
  - Configure Location and Prayer Calculation methods.
  - Update Display Messages and Mosque Name.
  - Adjust Brightness and Scroll Speed.
- **Hardware Support:**
  - ESP8266 (ESP-12E, NodeMCU, Wemos D1 Mini).
  - P10 LED Matrix Panels (via DMDESP library).
  - DS3231 RTC for timekeeping.
- **OTA Update:** Support for updating firmware over WiFi.

## Getting Started

### Prerequisites

- **Visual Studio Code** with **PlatformIO** extension.
- ESP8266 Driver installed on your computer.

### Hardware Setup

1. Connect the P10 LED Matrix to the ESP8266.
2. Connect the DS3231 RTC module (I2C interface).
3. Ensure adequate power supply (5V) for the LED Panels.

### Installation

1. Clone this repository or download the source code.
2. Open the project folder in VSCode.
3. PlatformIO should automatically install necessary libraries/dependencies.
4. Edit `platformio.ini` if you need to change the board type or upload speed.
5. Click **Build** to compile and **Upload** to flash the firmware.

## Usage

1. **Power On:** The device will create a WiFi Access Point (e.g., `Masjed_XXXXXX`).
2. **Connect:** Connect your phone or laptop to this WiFi network.
3. **Configure:** Open a web browser and navigate to `http://192.168.1.177`.
4. **Settings:** Use the web interface to set the current time, location coordinates, and display preferences.

## License

Personal/Open Source (Check license file if available).
