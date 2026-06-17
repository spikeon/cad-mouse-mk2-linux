#pragma once

#include "State.h"
#include <Arduino.h>

class ColorConfigState : public State {
 public:
  void enter() override;
  void update() override;
  void exit() override;

 private:
  enum class Step { Brightness, Color };

  Step step_;

  uint8_t workBrightness_;
  float   workHue_;       // 0.0 – 360.0

  uint8_t  origBrightness_;
  uint32_t origColor_;

  unsigned long lastLedMs_   = 0;

  float         rotAccum_    = 0;
  unsigned long lastUpdateMs_ = 0;

  void advanceStep();
  void cancelStep();
  void applyBreathing(unsigned long now);

  static uint32_t hueToRgb(float hue);
  static float    rgbToHue(uint32_t color);
};
