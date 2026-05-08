# Home Automation Android App (Kotlin, Wi-Fi, ESP8266)

This project is an Android application that lets you control home automation devices via Wi-Fi using voice commands. Designed for compatibility with ESP8266 modules or similar IoT boards, it leverages Android’s speech recognition and text-to-speech features to provide a natural user experience.

---

## Features

- **Voice-Controlled Automation:** Speak commands to control devices (lights, fans, etc.) through the app.
- **ESP8266 Integration:** Send commands directly to ESP8266-based devices over Wi-Fi.
- **Configurable Device IP:** Input and manage the target device's IP address from within the app.
- **Natural Feedback:** The app vocalizes device responses using text-to-speech.
- **User-Friendly Interface:** Start/stop listening with a single button and view both your spoken command and device response instantly.

---

## How It Works

1. **Speech Recognition:** Tap “Start” and speak your command. The app converts speech to text.
2. **Network Request:** The recognized text is sent as an HTTP request (GET) to the specified ESP8266 device IP.
3. **Device Response:** The app receives the response and vocalizes it while displaying on-screen feedback.

---

## Requirements

- Android device (API level supporting SpeechRecognizer and TextToSpeech).
- ESP8266 or similar Wi-Fi-enabled IoT hardware, running firmware to parse `/command?text=...` GET requests.

---

## Setup

1. Clone or download the project.
2. Open in Android Studio.
3. Connect your Android device or emulator.
4. Build and run the app.
5. Enter your ESP8266’s IP, tap “Start”, and issue voice commands for your devices.

---

## Example Use Cases

- Turn lights on/off by saying “Turn on the living room light”.
- Check status of home appliances with your voice.
- Expand features by customizing your ESP8266 firmware.

---

## Topics

- Android
- Kotlin
- Home Automation
- IoT
- ESP8266
- Wi-Fi Control
- Voice Recognition
- Smart Home
- Speech to Text

---

## License

[MIT License](LICENSE)
