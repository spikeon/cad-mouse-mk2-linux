#pragma once

#include <Arduino.h>

// Runtime LED configuration, persisted to flash via EEPROM emulation.
// Override defaults without reflashing: connect via USB serial and send:
//   led show               — print current settings
//   led brightness <0-255> — set brightness
//   led color <RRGGBB>     — set idle color (hex)
//   led reset              — restore firmware defaults
class LedConfig {
 public:
  uint8_t brightness;
  uint32_t idleColor;

  void load();
  void save();
  void reset();
};

extern LedConfig ledConfig;
