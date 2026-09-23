#include "BrightnessService.h"

#include <lvgl.h>
#include <time.h>
#include "ConfigService.h"

namespace
{
lv_obj_t *dimmer = nullptr;
uint8_t appliedBrightness = 255;

bool isNightTime()
{
    if (!ConfigService::get().nightModeEnabled) return false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return false;
    const unsigned int minute = timeinfo.tm_hour * 60U + timeinfo.tm_min;
    const DeviceConfig &config = ConfigService::get();
    if (config.nightStartMinute == config.nightEndMinute) return false;
    return config.nightStartMinute < config.nightEndMinute
        ? minute >= config.nightStartMinute && minute < config.nightEndMinute
        : minute >= config.nightStartMinute || minute < config.nightEndMinute;
}
}

uint8_t BrightnessService::effectiveBrightness()
{
    const DeviceConfig &config = ConfigService::get();
    return isNightTime() ? config.nightBrightness : config.brightness;
}

void BrightnessService::apply(lv_obj_t *screen)
{
    if (screen == nullptr) return;
    dimmer = lv_obj_create(screen);
    lv_obj_remove_style_all(dimmer);
    lv_obj_set_size(dimmer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(dimmer, lv_color_black(), 0);
    lv_obj_clear_flag(dimmer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dimmer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dimmer, LV_OBJ_FLAG_GESTURE_BUBBLE);
    appliedBrightness = 255;
    update();
}

void BrightnessService::update()
{
    if (dimmer == nullptr) return;
    const uint8_t brightness = effectiveBrightness();
    if (brightness == appliedBrightness) return;
    const uint8_t opacity = static_cast<uint8_t>(((100U - brightness) * 255U) / 100U);
    lv_obj_set_style_bg_opa(dimmer, opacity, 0);
    appliedBrightness = brightness;
}
