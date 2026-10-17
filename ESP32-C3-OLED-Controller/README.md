# ESP32-C3 OLED Controller

Two browser-controlled projects for an **ESP32-C3 SuperMini** and a **128×64 SH1106 I²C OLED display**.

## Projects

### 1. `oled_connected_to_home_wifi/oled_connected_to_home_wifi.ino` — Home Wi-Fi mode
Connects the ESP32-C3 to an existing 2.4 GHz Wi-Fi network. After connecting, the OLED displays the device's IP address. Open that address in a phone or computer browser on the same network to use the controller.

Features include:
- Send text to the OLED
- Draw with a mouse or touchscreen and send the drawing
- Select and stream a video as monochrome frames
- Clear the OLED

**Before uploading:** replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` in the sketch with your own Wi-Fi credentials. Do not publish real passwords in a public repository.

### 2. `sketch_oct16a/sketch_oct16a.ino` — ESP32 hotspot mode
Creates its own Wi-Fi network so a phone or computer can connect directly to the ESP32-C3.

Default access point settings in the sketch:
- SSID: `C3-OLED`
- Password: `oled12345`
- Web page: `http://192.168.4.1`

Features include:
- Select and stream video to the OLED
- Send text
- Draw with a mouse or touchscreen
- Clear the display

Change the hotspot password in the source code before sharing or using it in a setting where others may connect.

## Hardware required

- ESP32-C3 SuperMini development board
- 0.96-inch (or compatible) 128×64 OLED with **SH1106** controller and I²C interface
- Four jumper wires
- USB cable for programming and power

Check the controller printed on or specified for your OLED. These sketches use the U8g2 `SH1106` driver; an SSD1306 display may need a different driver configuration.

## Circuit diagram

```text
       ESP32-C3 SuperMini                  SH1106 OLED (I²C)
      ┌─────────────────┐                 ┌─────────────────┐
      │                 │                 │                 │
      │  3V3  ──────────┼─────────────────┤ VCC             │
      │  GND  ──────────┼─────────────────┤ GND             │
      │  GPIO 6 (SDA) ──┼─────────────────┤ SDA             │
      │  GPIO 7 (SCL) ──┼─────────────────┤ SCL             │
      │                 │                 │                 │
      └─────────────────┘                 └─────────────────┘
```

## Connection table

| ESP32-C3 SuperMini | OLED module |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO 6 | SDA |
| GPIO 7 | SCL |

**Important:** Use the board's **3.3 V** pin for this wiring. Turn off power before changing connections. Check your particular OLED module's pin labels before connecting it.

## Arduino IDE setup

1. Install the ESP32 board package in Arduino IDE and select an ESP32-C3 board compatible with your SuperMini.
2. Install the **U8g2** library from **Tools → Manage Libraries** (or **Sketch → Include Library → Manage Libraries**, depending on IDE version).
3. Open one `.ino` file at a time. Each sketch is a separate program; do not combine both into one sketch.
4. For `oled_connected_to_home_wifi/oled_connected_to_home_wifi.ino`, enter your Wi-Fi SSID and password in the two configuration lines. Use a 2.4 GHz network if your router offers separate bands.
5. Connect the ESP32-C3 using USB, select the correct port, and click **Upload**.
6. Open **Tools → Serial Monitor** and set the baud rate to **115200**.
7. For home Wi-Fi mode, read the IP address printed in Serial Monitor or shown on the OLED. Open `http://<ESP32-IP-address>` in a browser connected to the same network.
8. For hotspot mode, connect your phone/computer to `C3-OLED` using the password shown in the sketch, then open `http://192.168.4.1`.

## Troubleshooting

- **Blank display:** confirm VCC, GND, SDA, and SCL; verify that the OLED is SH1106-compatible and wired to GPIO 6/7.
- **Home Wi-Fi connection fails:** verify the SSID/password and ensure the network supports 2.4 GHz Wi-Fi.
- **Web page does not open:** make sure your phone/computer is connected to the same network as the ESP32, or to the ESP32 hotspot for hotspot mode.
- **Video is unsupported or choppy:** try a browser-supported MP4/H.264 file and keep in mind that a 128×64 monochrome display has very limited resolution and transfer bandwidth.
- **Text or drawings disappear during video:** stop the video stream before sending text or drawings.

## Security notes

- The home-Wi-Fi sketch in this repository uses placeholders instead of private credentials. Enter your credentials locally before compiling.
- The hotspot sketch includes a default password in source code. Change it if you need stronger access control.
- These sketches use an HTTP web server without authentication. Use them on a trusted local network and avoid exposing the device directly to the public internet.

## License

This project is released under the MIT License. See [`LICENSE`](LICENSE).
