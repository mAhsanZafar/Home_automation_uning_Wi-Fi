#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const bool RELAY_ACTIVE_LOW = true;

struct Device {
  const char *name;
  uint8_t pin;
  bool state;
};

Device devices[] = {
  {"light", D1, false},
  {"fan", D2, false},
};

ESP8266WebServer server(80);

String urlDecode(const String &input) {
  String output;
  output.reserve(input.length());

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '+') {
      output += ' ';
      continue;
    }

    if (c == '%' && i + 2 < input.length()) {
      char hex[3] = {input[i + 1], input[i + 2], '\0'};
      char decoded = static_cast<char>(strtol(hex, nullptr, 16));
      output += decoded;
      i += 2;
      continue;
    }

    output += c;
  }

  return output;
}

bool containsAny(const String &text, const char *const *keywords, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (text.indexOf(keywords[i]) >= 0) {
      return true;
    }
  }
  return false;
}

bool wantsOn(const String &text) {
  const char *keywords[] = {"turn on", "switch on", "on", "start"};
  return containsAny(text, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

bool wantsOff(const String &text) {
  const char *keywords[] = {"turn off", "switch off", "off", "stop"};
  return containsAny(text, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

void setDeviceState(Device &device, bool on) {
  uint8_t relayOnValue = RELAY_ACTIVE_LOW ? LOW : HIGH;
  uint8_t relayOffValue = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(device.pin, on ? relayOnValue : relayOffValue);
  device.state = on;
}

String buildStatus() {
  String status;
  for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
    status += devices[i].name;
    status += ": ";
    status += devices[i].state ? "on" : "off";
    if (i + 1 < sizeof(devices) / sizeof(devices[0])) {
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
  server.send(200, "text/plain", buildStatus());
}

void handleCommand() {
  if (!server.hasArg("text")) {
    server.send(400, "text/plain", "Missing 'text' query parameter.");
    return;
  }

  String command = urlDecode(server.arg("text"));
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    server.send(400, "text/plain", "Command is empty.");
    return;
  }

  if (command.indexOf("status") >= 0 || command.indexOf("state") >= 0) {
    server.send(200, "text/plain", buildStatus());
    return;
  }

  bool turnOn = wantsOn(command);
  bool turnOff = wantsOff(command);

  if (!turnOn && !turnOff) {
    server.send(200, "text/plain",
                "Please say on/off. Example: light on, fan off, all on.");
    return;
  }

  if (command.indexOf("all") >= 0) {
    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
      setDeviceState(devices[i], turnOn);
    }
    server.send(200, "text/plain", turnOn ? "All devices turned on."
                                          : "All devices turned off.");
    return;
  }

  for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
    if (command.indexOf(devices[i].name) >= 0) {
      setDeviceState(devices[i], turnOn);
      String response = String(devices[i].name) + (turnOn ? " turned on."
                                                          : " turned off.");
      server.send(200, "text/plain", response);
      return;
    }
  }

  server.send(200, "text/plain",
              "Unknown device. Try: light on, fan off, all on.");
}

void setupDevices() {
  for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
    pinMode(devices[i].pin, OUTPUT);
    setDeviceState(devices[i], false);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  setupDevices();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed.");
  }

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
  server.handleClient();
}
