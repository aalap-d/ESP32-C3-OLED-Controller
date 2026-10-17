# ESP32-C3 OLED Controller

Two ESP32-C3 SuperMini projects for controlling a 128×64 SH1106 I²C OLED display over Wi-Fi.

## Projects

### 1. Home Wi-Fi mode
Sketch: [`oled_connected_to_home_wifi/oled_connected_to_home_wifi.ino`](oled_connected_to_home_wifi/oled_connected_to_home_wifi.ino)

Connects the ESP32-C3 to an existing Wi-Fi network and provides a browser interface for the features implemented in the sketch.

### 2. Hotspot mode
Sketch: [`sketch_oct16a/sketch_oct16a.ino`](sketch_oct16a/sketch_oct16a.ino)

Creates its own Wi-Fi access point so a phone or computer can connect directly to the ESP32-C3.

## Hardware required

- ESP32-C3 SuperMini
- 128×64 SH1106 OLED display with I²C interface
- Four jumper wires
- USB cable for programming and power

## Wiring diagram

The following diagram shows the four I²C connections for this project.

![ESP32-C3 SuperMini to SH1106 OLED wiring diagram](images/esp32-c3-oled-wiring.png)

### Pin connections

| ESP32-C3 SuperMini | SH1106 OLED |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO 6 | SDA |
| GPIO 7 | SCL |

**Important:** This is an I²C OLED wiring diagram, not a parallel TFT wiring diagram. Check the labels on your exact OLED module before connecting power. Use 3.3 V logic and do not connect 5 V to 3.3 V-only inputs.

## Arduino IDE setup

1. Install Arduino IDE.
2. Install the ESP32 board package using Boards Manager.
3. Install the required display library used by your sketch (for example, U8g2 if the source includes it).
4. Open one `.ino` file at a time; these are separate programs.
5. Select a compatible ESP32-C3 board and the correct COM port.
6. For home Wi-Fi mode, replace the Wi-Fi placeholders locally with your own network credentials.
7. Upload the sketch and follow the IP address or hotspot details shown by the program.

## Security

- Do not publish real Wi-Fi passwords in a public repository.
- Change any default hotspot password before sharing the project.
- Avoid exposing the ESP32 web server directly to the public internet.

## Repository structure

```text
ESP32-C3-OLED-Controller/
├── README.md
├── LICENSE
├── images/
│   └── esp32-c3-oled-wiring.png
├── oled_connected_to_home_wifi/
│   └── oled_connected_to_home_wifi.ino
└── sketch_oct16a/
    └── sketch_oct16a.ino
```

## License

Released under the MIT License. See [LICENSE](LICENSE).
