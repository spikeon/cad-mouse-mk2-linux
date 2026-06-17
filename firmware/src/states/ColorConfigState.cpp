#include "states/ColorConfigState.h"

#include <Arduino.h>
#include <math.h>

#include "Controllers.h"
#include "LedConfig.h"
#include "StateMachine.h"

namespace {
const float kRotTickThreshold        = 1200.0f;
const uint8_t kBrightnessStep        = 8;
const float kHueStep                 = 5.0f;
const unsigned long kBreathePeriodMs = 2000;
const unsigned long kBreathUpdateMs  = 16;   // ~60fps
}

// ── Color helpers ─────────────────────────────────────────────────────────────

uint32_t ColorConfigState::hueToRgb(float hue) {
  hue = fmod(hue, 360.0f);
  if (hue < 0) hue += 360.0f;
  float x = 1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f);
  float r = 0, g = 0, b = 0;
  if      (hue < 60)  { r = 1; g = x; b = 0; }
  else if (hue < 120) { r = x; g = 1; b = 0; }
  else if (hue < 180) { r = 0; g = 1; b = x; }
  else if (hue < 240) { r = 0; g = x; b = 1; }
  else if (hue < 300) { r = x; g = 0; b = 1; }
  else                { r = 1; g = 0; b = x; }
  return ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | (uint8_t)(b * 255);
}

float ColorConfigState::rgbToHue(uint32_t color) {
  float r = ((color >> 16) & 0xFF) / 255.0f;
  float g = ((color >> 8)  & 0xFF) / 255.0f;
  float b = ( color        & 0xFF) / 255.0f;
  float mx = max(r, max(g, b));
  float mn = min(r, min(g, b));
  float delta = mx - mn;
  if (delta < 0.01f) return 0;
  float h;
  if      (mx == r) h = 60.0f * fmodf((g - b) / delta, 6.0f);
  else if (mx == g) h = 60.0f * ((b - r) / delta + 2.0f);
  else              h = 60.0f * ((r - g) / delta + 4.0f);
  if (h < 0) h += 360.0f;
  return h;
}

// ── State lifecycle ───────────────────────────────────────────────────────────

void ColorConfigState::enter() {
  origBrightness_ = ledConfig.brightness;
  origColor_      = ledConfig.idleColor;

  workBrightness_ = ledConfig.brightness;
  workHue_        = rgbToHue(ledConfig.idleColor);

  step_         = Step::Brightness;
  rotAccum_     = 0;
  lastLedMs_    = 0;
  lastUpdateMs_ = 0;

  // Clear any stale clicks that accumulated during hold
  inputController.takeLeftClick();
  inputController.takeRightClick();
}

void ColorConfigState::exit() {
  // Restore saved config brightness so LEDs look correct on re-enter
  ledController.applyConfig();
}

// ── Breathing effect ──────────────────────────────────────────────────────────

void ColorConfigState::applyBreathing(unsigned long now) {
  const float t = (now % kBreathePeriodMs) / (float)kBreathePeriodMs;
  const float factor = (sinf(t * 2.0f * (float)M_PI - (float)M_PI / 2.0f) + 1.0f) / 2.0f;
  const uint8_t displayBrightness = (uint8_t)(workBrightness_ * factor + 0.5f);
  const uint32_t color = (step_ == Step::Brightness) ? ledConfig.idleColor : hueToRgb(workHue_);
  ledController.setPreview(displayBrightness, color);
}

// ── Step transitions ──────────────────────────────────────────────────────────

void ColorConfigState::advanceStep() {
  if (step_ == Step::Brightness) {
    // Save brightness, move to color step
    ledConfig.brightness = workBrightness_;
    workHue_ = rgbToHue(ledConfig.idleColor);
    rotAccum_ = 0;
    step_ = Step::Color;
    // Clear clicks
    inputController.takeLeftClick();
    inputController.takeRightClick();
  } else {
    // Save color and brightness, exit
    ledConfig.brightness = workBrightness_;
    ledConfig.idleColor  = hueToRgb(workHue_);
    ledConfig.save();
    stateMachine.changeState(&StateMachine::idleState);
  }
}

void ColorConfigState::cancelStep() {
  if (step_ == Step::Brightness) {
    // Discard brightness change, exit entirely
    workBrightness_ = origBrightness_;
  } else {
    // Discard color change; brightness was already saved when we advanced
    // Restore original brightness too since nothing was committed yet
    ledConfig.brightness = origBrightness_;
    ledConfig.idleColor  = origColor_;
  }
  stateMachine.changeState(&StateMachine::idleState);
}

// ── Main update ───────────────────────────────────────────────────────────────

void ColorConfigState::update() {
  inputController.update();

  const unsigned long now = millis();
  const float dt = (lastUpdateMs_ == 0) ? 0.01f
                                        : ((now - lastUpdateMs_) / 1000.0f);
  lastUpdateMs_ = now;

  // ── Button handling ────────────────────────────────────────────────────────
  if (inputController.takeRightClick()) {
    advanceStep();
    return;
  }
  if (inputController.takeLeftClick()) {
    cancelStep();
    return;
  }

  // ── Rotation input ─────────────────────────────────────────────────────────
  float raw[9] = {};
  sensorController.readRaw(raw);
  float motion[6] = {};
  motionController.compute(raw, sensorController.baseline(), dt, motion);

  rotAccum_ += motion[5];  // Rz axis

  bool changed = false;
  while (rotAccum_ >= kRotTickThreshold) {
    rotAccum_ -= kRotTickThreshold;
    if (step_ == Step::Brightness) {
      workBrightness_ = (uint8_t)min(255, (int)workBrightness_ + kBrightnessStep);
    } else {
      workHue_ = fmod(workHue_ + kHueStep + 360.0f, 360.0f);
    }
    changed = true;
  }
  while (rotAccum_ <= -kRotTickThreshold) {
    rotAccum_ += kRotTickThreshold;
    if (step_ == Step::Brightness) {
      workBrightness_ = (uint8_t)max(0, (int)workBrightness_ - kBrightnessStep);
    } else {
      workHue_ = fmod(workHue_ - kHueStep + 360.0f, 360.0f);
    }
    changed = true;
  }

  // ── Breathing LED ──────────────────────────────────────────────────────────
  if (changed || (now - lastLedMs_) >= kBreathUpdateMs) {
    lastLedMs_ = now;
    applyBreathing(now);
  }
}
