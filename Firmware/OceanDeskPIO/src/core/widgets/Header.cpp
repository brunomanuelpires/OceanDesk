#include "Header.h"
#include "../../config/Theme.h"
#include "../../services/ClockService.h"
#include "../../services/WiFiService.h"
#include <lvgl.h>

lv_obj_t *Header::timeLabel = nullptr;
lv_obj_t *Header::dateLabel = nullptr;
lv_obj_t *Header::networkLabel = nullptr;
namespace { lv_timer_t *updateTimer = nullptr; }

void Header::create(lv_obj_t *parent)
{
    // Título
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "OceanDesk");

    lv_obj_align(
        title,
        LV_ALIGN_TOP_LEFT,
        22,
        18
    );

    lv_obj_set_style_text_font(
        title,
        Theme::FontMedium,
        0
    );

    lv_obj_set_style_text_color(
        title,
        Theme::color(Theme::Text),
        0
    );

    networkLabel = lv_label_create(parent);
    lv_label_set_text(networkLabel, "");
    lv_obj_align(networkLabel, LV_ALIGN_TOP_RIGHT, -22, 22);
    lv_obj_set_style_text_font(networkLabel, Theme::FontSmall, 0);
    lv_obj_set_style_text_color(networkLabel, Theme::color(Theme::Accent), 0);

    // Hora
    timeLabel = lv_label_create(parent);
    lv_label_set_text(timeLabel, "--:--");

    lv_obj_align(
        timeLabel,
        LV_ALIGN_TOP_MID,
        0,
        48
    );

    lv_obj_set_style_text_font(
        timeLabel,
        Theme::FontTime,
        0
    );

    lv_obj_set_style_text_color(
        timeLabel,
        Theme::color(Theme::Text),
        0
    );

    // Data
    dateLabel = lv_label_create(parent);
    lv_label_set_text(dateLabel, "--");

    lv_obj_align(
        dateLabel,
        LV_ALIGN_TOP_MID,
        0,
        158
    );

    lv_obj_set_style_text_font(
        dateLabel,
        Theme::FontMedium,
        0
    );

    lv_obj_set_style_text_color(
        dateLabel,
        Theme::color(Theme::Text),
        0
    );

    // Preenche logo os valores
    refresh();

    // Atualiza o Header a cada segundo
    if (updateTimer == nullptr) updateTimer = lv_timer_create(timerCallback, 1000, nullptr);
}

void Header::clear()
{
    timeLabel = nullptr;
    dateLabel = nullptr;
    networkLabel = nullptr;
}

void Header::refresh()
{
    if (timeLabel == nullptr || dateLabel == nullptr || networkLabel == nullptr)
    {
        return;
    }

    String currentTime = ClockService::getTime();

    String currentDate =
        ClockService::getWeekday() +
        ", " +
        ClockService::getDate();

    lv_label_set_text(
        timeLabel,
        currentTime.c_str()
    );

    lv_label_set_text(
        dateLabel,
        currentDate.c_str()
    );

    lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_align(dateLabel, LV_ALIGN_TOP_MID, 0, 158);
    lv_label_set_text(networkLabel, WiFiService::isOffline() ? "Sem Wi-Fi" : "");
}

void Header::timerCallback(lv_timer_t *timer)
{
    (void)timer;

    refresh();
}
