#include "HomeScreen.h"
#include <lvgl.h>
#include "config/Theme.h"
#include "../../services/TideService.h"
#include "../../services/ConfigService.h"
#include "../../services/AlarmService.h"
#include "HomeScreenLayout.h"

namespace
{
HomeScreenLayout *activeLayout = nullptr;
lv_obj_t *alarmOverlay = nullptr;

void dismissAlarm(lv_event_t *event)
{
    (void)event;
    AlarmService::requestDismiss();
}

void createAlarmOverlay(lv_obj_t *screen)
{
    alarmOverlay = lv_obj_create(screen);
    lv_obj_remove_style_all(alarmOverlay);
    lv_obj_set_size(alarmOverlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(alarmOverlay, Theme::color(Theme::Background), 0);
    lv_obj_set_style_bg_opa(alarmOverlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(alarmOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(alarmOverlay);

    lv_obj_t *title = lv_label_create(alarmOverlay);
    lv_label_set_text(title, "ALARME");
    lv_obj_set_style_text_font(title, Theme::FontXL, 0);
    lv_obj_set_style_text_color(title, Theme::color(Theme::Accent), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 58);

    const DeviceConfig &config = ConfigService::get();
    lv_obj_t *time = lv_label_create(alarmOverlay);
    lv_label_set_text_fmt(time, "%02u:%02u", config.alarmHour, config.alarmMinute);
    lv_obj_set_style_text_font(time, Theme::FontTime, 0);
    lv_obj_set_style_text_color(time, Theme::color(Theme::Text), 0);
    lv_obj_align(time, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *button = lv_btn_create(alarmOverlay);
    lv_obj_set_size(button, 330, 76);
    lv_obj_set_style_radius(button, 18, 0);
    lv_obj_set_style_bg_color(button, Theme::color(Theme::Accent), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_obj_add_event_cb(button, dismissAlarm, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *buttonLabel = lv_label_create(button);
    lv_label_set_text(buttonLabel, "Parar alarme");
    lv_obj_set_style_text_font(buttonLabel, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(buttonLabel, Theme::color(Theme::Background), 0);
    lv_obj_center(buttonLabel);
}
}

void HomeScreen::create()
{
    lv_obj_t *screen = lv_scr_act();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, Theme::color(Theme::Background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    alarmOverlay = nullptr;
    activeLayout = &homeScreenLayoutFor(ConfigService::get().displayStyle);
    activeLayout->create(screen);
    refresh();
}

void HomeScreen::refresh()
{
    if (AlarmService::isRinging())
    {
        if (alarmOverlay == nullptr)
        {
            createAlarmOverlay(lv_scr_act());
        }
        return;
    }

    if (alarmOverlay != nullptr)
    {
        lv_obj_del(alarmOverlay);
        alarmOverlay = nullptr;
    }

    if (activeLayout != nullptr)
    {
        activeLayout->refresh(TideService::getSnapshot());
    }
}
