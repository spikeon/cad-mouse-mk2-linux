#include "LedConfig.h"
#include "Config.h"
#include <EEPROM.h>

LedConfig ledConfig;

namespace {
constexpr uint32_t kMagic    = 0xCAD10001;
constexpr int      kSize     = 16;
constexpr int      kAddrMagic      = 0;
constexpr int      kAddrBrightness = 4;
constexpr int      kAddrColor      = 5;
}

void LedConfig::load() {
  EEPROM.begin(kSize);
  uint32_t magic = 0;
  EEPROM.get(kAddrMagic, magic);
  if (magic != kMagic) {
    reset();
    return;
  }
  EEPROM.get(kAddrBrightness, brightness);
  uint32_t color = 0;
  EEPROM.get(kAddrColor, color);
  idleColor = color;
}

void LedConfig::save() {
  EEPROM.begin(kSize);
  uint32_t magic = kMagic;
  EEPROM.put(kAddrMagic, magic);
  EEPROM.put(kAddrBrightness, brightness);
  uint32_t color = idleColor;
  EEPROM.put(kAddrColor, color);
  EEPROM.commit();
}

void LedConfig::reset() {
  brightness = Config::LED_BRIGHTNESS;
  idleColor  = Config::LED_IDLE_COLOR;
}
