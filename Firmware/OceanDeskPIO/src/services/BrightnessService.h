#pragma once

#include <Arduino.h>
struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

// The Waveshare backlight is wired through a CH422G switch, so it cannot be
// PWM dimmed. This service provides a non-invasive visual dimmer instead.
class BrightnessService
{
public:
    static void apply(lv_obj_t *screen);
    static void update();
    static uint8_t effectiveBrightness();
};
