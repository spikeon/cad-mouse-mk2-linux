#include <Arduino.h>

#include "Config.h"
#include "Controllers.h"
#include "StateMachine.h"

InputController inputController;
LEDController ledController;
SensorController sensorController;
MotionController motionController;
HIDController hidController;
TelemetryController telemetryController;

void setup() {
  // Check for bootloader mode before any USB init: plug in while holding both buttons
  pinMode(Config::PIN_LEFT_BTN, INPUT_PULLUP);
  pinMode(Config::PIN_RIGHT_BTN, INPUT_PULLUP);
  if (digitalRead(Config::PIN_LEFT_BTN) == LOW && digitalRead(Config::PIN_RIGHT_BTN) == LOW) {
    rp2040.rebootToBootloader();
  }

  // Initialize USB HID first
  hidController.begin();

  if (Config::ENABLE_TELEMETRY) {
    Serial.begin(115200);
    delay(200);
  }

  inputController.begin();
  ledController.begin();
  sensorController.begin();
  motionController.reset();
  telemetryController.begin();

  stateMachine.changeState(&StateMachine::calibratingState);
}

void loop() {
  hidController.task();
  if (inputController.takeBootloaderRequest()) {
    rp2040.rebootToBootloader();
  }
  stateMachine.update();
}
