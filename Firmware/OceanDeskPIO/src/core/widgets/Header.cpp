#include "Header.h"

#include "../../config/Theme.h"
#include "../../services/ClockService.h"

#include <lvgl.h>

lv_obj_t *Header::timeLabel = nullptr;
lv_obj_t *Header::dateLabel = nullptr;

void Header::create()
{
    lv_obj_t *screen = lv_scr_act();

    // Título
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "OceanDesk");

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        15
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

    // Hora
    timeLabel = lv_label_create(screen);

    lv_obj_align(
        timeLabel,
        LV_ALIGN_TOP_MID,
        0,
        55
    );

    lv_obj_set_style_text_font(
        timeLabel,
        Theme::FontXL,
        0
    );

    lv_obj_set_style_text_color(
        timeLabel,
        Theme::color(Theme::Text),
        0
    );

    // Data
    dateLabel = lv_label_create(screen);

    lv_obj_align(
        dateLabel,
        LV_ALIGN_TOP_MID,
        0,
        115
    );

    lv_obj_set_style_text_font(
        dateLabel,
        Theme::FontSmall,
        0
    );

    lv_obj_set_style_text_color(
        dateLabel,
        Theme::color(Theme::TextMuted),
        0
    );

    // Preenche logo os valores
    refresh();

    // Atualiza o Header a cada segundo
    lv_timer_create(
        timerCallback,
        1000,
        nullptr
    );
}

void Header::refresh()
{
    if (timeLabel == nullptr || dateLabel == nullptr)
    {
        return;
    }

    char timeBuffer[6];
    char dateBuffer[64];

    ClockService::getTime(
        timeBuffer,
        sizeof(timeBuffer)
    );

    ClockService::getDate(
        dateBuffer,
        sizeof(dateBuffer)
    );

    lv_label_set_text(
        timeLabel,
        timeBuffer
    );

    lv_label_set_text(
        dateLabel,
        dateBuffer
    );
}

void Header::timerCallback(lv_timer_t *timer)
{
    (void)timer;

    refresh();
}