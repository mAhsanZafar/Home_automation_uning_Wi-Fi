# Home Automation Using Wi‑Fi (ESP8266 + Android)

This project pairs an Android voice‑control app with an ESP8266 web server. The app sends a spoken command to the ESP8266 over Wi‑Fi, and the ESP8266 switches relays for devices like a light or fan.

## Repository layout

- `Android projects/` — Android Studio project (voice recognition + HTTP request).
- `esp8266/HomeAutomationESP8266.ino` — ESP8266 sketch that receives commands and toggles relays.

## ESP8266 setup

1. Open `esp8266/HomeAutomationESP8266.ino` in the Arduino IDE.
2. Install the ESP8266 board package in **Tools → Board → Boards Manager**.
3. Copy `esp8266/config.example.h` to `esp8266/config.h` and update Wi‑Fi credentials:
   ```cpp
   const char *WIFI_SSID = "YOUR_WIFI_SSID";
   const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```
4. (Optional) Set an API key in `config.h` to protect `/command` and `/status`.
5. Adjust relay pins if needed:
   ```cpp
   Device devices[] = {
     {"light", D1, false},
     {"fan", D2, false},
   };
   ```
6. Flash the sketch to the ESP8266 (NodeMCU, Wemos D1 mini, etc.).
7. Open Serial Monitor at **115200 baud** to see the assigned IP address.

### Wiring notes

- Relays are assumed **active‑LOW** (`RELAY_ACTIVE_LOW = true`). If your relay is active‑HIGH, set it to `false`.
- Connect relay IN pins to the configured GPIOs (D1, D2, etc.) and use appropriate power for the relay module.

## Android app setup

1. Open `Android projects/` in Android Studio.
2. Build and run on a device.
3. Enter the ESP8266 IP address in the app.
4. Tap **Start** and speak commands.

## Supported commands

Examples (case‑insensitive):

- **Light**: “light on”, “light off”
- **Fan**: “fan on”, “fan off”
- **All devices**: “all on”, “all off”
- **Status**: “status”, “state”

The ESP8266 responds with a short text message, which the app will display and read aloud.

> If you set `API_KEY`, use `?key=YOUR_KEY` on `/status` and `&key=YOUR_KEY` on `/command?text=...` (or keep it empty for the current Android app).

## Troubleshooting

- Ensure the phone and ESP8266 are on the same Wi‑Fi network.
- Confirm the IP address in the app matches the ESP8266 Serial Monitor output.
- If you changed device names in the sketch, update your spoken commands accordingly.
