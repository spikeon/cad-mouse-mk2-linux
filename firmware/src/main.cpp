#include <Arduino.h>

#include "Config.h"
#include "Controllers.h"
#include "LedConfig.h"
#include "StateMachine.h"

InputController inputController;
LEDController ledController;
SensorController sensorController;
MotionController motionController;
HIDController hidController;
TelemetryController telemetryController;

// ── Serial LED command handler ────────────────────────────────────────────────
// Connect via USB serial and send:
//   led show               — print current settings
//   led brightness <0-255> — set brightness
//   led color <RRGGBB>     — set idle color (hex)
//   led reset              — restore firmware defaults

namespace {
String serialBuf;

void handleLedCommand(const String& args) {
  if (args == "show") {
    char buf[48];
    snprintf(buf, sizeof(buf), "brightness=%d color=%06lX\n",
             ledConfig.brightness, (unsigned long)ledConfig.idleColor);
    Serial.print(buf);
    return;
  }

  if (args == "reset") {
    ledConfig.reset();
    ledConfig.save();
    ledController.applyConfig();
    Serial.println("OK reset");
    return;
  }

  if (args.startsWith("brightness ")) {
    int val = args.substring(11).toInt();
    if (val < 0 || val > 255) { Serial.println("ERR brightness 0-255"); return; }
    ledConfig.brightness = (uint8_t)val;
    ledConfig.save();
    ledController.applyConfig();
    Serial.println("OK");
    return;
  }

  if (args.startsWith("color ")) {
    String hex = args.substring(6);
    hex.trim();
    if (hex.length() != 6) { Serial.println("ERR color RRGGBB"); return; }
    ledConfig.idleColor = (uint32_t)strtoul(hex.c_str(), nullptr, 16);
    ledConfig.save();
    ledController.applyConfig();
    Serial.println("OK");
    return;
  }

  Serial.println("ERR unknown: led show|brightness N|color RRGGBB|reset");
}

void handleSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      serialBuf.trim();
      if (serialBuf.startsWith("led ")) {
        handleLedCommand(serialBuf.substring(4));
      }
      serialBuf = "";
    } else {
      serialBuf += c;
    }
  }
}
}  // namespace

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
  // Check for bootloader mode before any USB init: plug in while holding both buttons
  pinMode(Config::PIN_LEFT_BTN, INPUT_PULLUP);
  pinMode(Config::PIN_RIGHT_BTN, INPUT_PULLUP);
  if (digitalRead(Config::PIN_LEFT_BTN) == LOW && digitalRead(Config::PIN_RIGHT_BTN) == LOW) {
    rp2040.rebootToBootloader();
  }

  // Initialize USB HID first
  hidController.begin();

  // Serial is always enabled for LED config commands; also used for telemetry
  Serial.begin(115200);
  if (Config::ENABLE_TELEMETRY) {
    delay(200);
  }

  ledConfig.load();

  inputController.begin();
  ledController.begin();
  sensorController.begin();
  motionController.reset();
  telemetryController.begin();

  stateMachine.changeState(&StateMachine::calibratingState);
}

void loop() {
  hidController.task();
  handleSerial();
  stateMachine.update();
}
