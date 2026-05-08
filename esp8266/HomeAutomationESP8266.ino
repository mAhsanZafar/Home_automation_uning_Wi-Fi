#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ctype.h>

#if __has_include("config.h")
#include "config.h"
#else
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *API_KEY = "";
#endif

const bool RELAY_ACTIVE_LOW = true;
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;
const unsigned long WIFI_RETRY_DELAY_MS = 5000;
const unsigned long WIFI_CONNECT_POLL_DELAY_MS = 250;
const unsigned long SERIAL_INIT_DELAY_MS = 200;

struct Device {
  const char *name;
  uint8_t pin;
  bool state;
};

Device devices[] = {
  {"light", D1, false},
  {"fan", D2, false},
};
const size_t DEVICE_COUNT = sizeof(devices) / sizeof(devices[0]);

ESP8266WebServer server(80);
unsigned long lastReconnectAttempt = 0;

char normalizeChar(char value) {
  return (value == '\t' || value == '\n' || value == '\r') ? ' ' : value;
}

bool isAllowedChar(char value) {
  return isPrintable(value) || value == '\t' || value == '\n' || value == '\r';
}

bool urlDecode(const String &input, String &output) {
  output.reserve(input.length());

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '+') {
      output += ' ';
      continue;
    }

    if (c == '%' && i + 2 < input.length() &&
        isxdigit(static_cast<unsigned char>(input[i + 1])) &&
        isxdigit(static_cast<unsigned char>(input[i + 2]))) {
      char hex[3] = {input[i + 1], input[i + 2], '\0'};
      char decoded = static_cast<char>(strtol(hex, nullptr, 16));
      if (!isAllowedChar(decoded)) {
        return false;
      }
      output += normalizeChar(decoded);
      i += 2;
      continue;
    }

    if (!isAllowedChar(c)) {
      return false;
    }
    output += normalizeChar(c);
  }

  return true;
}

bool containsAny(const String &text, const char *const *keywords, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (text.indexOf(keywords[i]) >= 0) {
      return true;
    }
  }
  return false;
}

bool isWordChar(char ch) {
  unsigned char uch = static_cast<unsigned char>(ch);
  return isalnum(uch);
}

bool containsWord(const String &text, const String &word) {
  int index = text.indexOf(word);
  int textLength = static_cast<int>(text.length());
  int wordLength = static_cast<int>(word.length());
  while (index >= 0) {
    int before = index - 1;
    int after = index + wordLength;
    bool beforeOk = before < 0 || !isWordChar(text[before]);
    bool afterOk = after >= textLength || !isWordChar(text[after]);
    if (beforeOk && afterOk) {
      return true;
    }
    index = text.indexOf(word, index + wordLength);
  }
  return false;
}

bool wantsOn(const String &text) {
  const char *phrases[] = {"turn on", "switch on"};
  if (containsAny(text, phrases, sizeof(phrases) / sizeof(phrases[0]))) {
    return true;
  }
  return containsWord(text, "on") || containsWord(text, "start");
}

bool wantsOff(const String &text) {
  const char *phrases[] = {"turn off", "switch off"};
  if (containsAny(text, phrases, sizeof(phrases) / sizeof(phrases[0]))) {
    return true;
  }
  return containsWord(text, "off") || containsWord(text, "stop");
}

void setDeviceState(Device &device, bool on) {
  uint8_t relayOnValue = RELAY_ACTIVE_LOW ? LOW : HIGH;
  uint8_t relayOffValue = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(device.pin, on ? relayOnValue : relayOffValue);
  device.state = on;
}

String buildStatus() {
  String status;
  for (size_t i = 0; i < DEVICE_COUNT; i++) {
    status += devices[i].name;
    status += ": ";
    status += devices[i].state ? "on" : "off";
    if (i + 1 < DEVICE_COUNT) {
      status += "\n";
    }
  }
  return status;
}

void handleRoot() {
  server.send(200, "text/plain",
              "ESP8266 Home Automation is running.\n"
              "Use /command?text=your%20command or /status.");
}

void handleStatus() {
  if (strlen(API_KEY) > 0 && server.arg("key") != API_KEY) {
    server.send(401, "text/plain", "Unauthorized.");
    return;
  }
  server.send(200, "text/plain", buildStatus());
}

void handleCommand() {
  if (strlen(API_KEY) > 0 && server.arg("key") != API_KEY) {
    server.send(401, "text/plain", "Unauthorized.");
    return;
  }
  if (!server.hasArg("text")) {
    server.send(400, "text/plain", "Missing 'text' query parameter.");
    return;
  }

  String command;
  if (!urlDecode(server.arg("text"), command)) {
    server.send(400, "text/plain", "Command contains invalid characters.");
    return;
  }
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    server.send(400, "text/plain", "Command is empty.");
    return;
  }

  if (containsWord(command, "status") || containsWord(command, "state")) {
    server.send(200, "text/plain", buildStatus());
    return;
  }

  bool turnOn = wantsOn(command);
  bool turnOff = wantsOff(command);

  if (!turnOn && !turnOff) {
    server.send(400, "text/plain",
                "Please say on/off. Example: light on, fan off, all on.");
    return;
  }

  if (containsWord(command, "all")) {
    for (size_t i = 0; i < DEVICE_COUNT; i++) {
      setDeviceState(devices[i], turnOn);
    }
    server.send(200, "text/plain", turnOn ? "All devices turned on."
                                          : "All devices turned off.");
    return;
  }

  for (size_t i = 0; i < DEVICE_COUNT; i++) {
    if (containsWord(command, devices[i].name)) {
      setDeviceState(devices[i], turnOn);
      String response = String(devices[i].name) + (turnOn ? " turned on."
                                                          : " turned off.");
      server.send(200, "text/plain", response);
      return;
    }
  }

  server.send(400, "text/plain",
              "Unknown device. Try: light on, fan off, all on.");
}

void setupDevices() {
  for (size_t i = 0; i < DEVICE_COUNT; i++) {
    pinMode(devices[i].pin, OUTPUT);
    setDeviceState(devices[i], false);
  }
}

bool connectWiFiOnce() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_CONNECT_POLL_DELAY_MS);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println();
  Serial.println("WiFi connection failed. Will retry.");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(SERIAL_INIT_DELAY_MS);

  setupDevices();

  connectWiFiOnce();
  lastReconnectAttempt = millis();

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/command", handleCommand);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found.");
  });

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnectAttempt >= WIFI_RETRY_DELAY_MS) {
    lastReconnectAttempt = millis();
    connectWiFiOnce();
  }
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}
