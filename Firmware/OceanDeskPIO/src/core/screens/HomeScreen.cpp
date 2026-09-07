#include "HomeScreen.h"
#include <lvgl.h>
#include "fonts/Fonts.h"
#include "config/Theme.h"
#include "../widgets/Header.h"
#include "../../services/TideService.h"

namespace
{
lv_obj_t *status = nullptr;
lv_obj_t *lowTime = nullptr;
lv_obj_t *highTime = nullptr;
}

void HomeScreen::create()
{
    lv_obj_t *screen = lv_scr_act();

    // Limpar o ecrã
    lv_obj_clean(screen);

    // Fundo
    lv_obj_set_style_bg_color(screen, Theme::color(Theme::Background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    // Header: OceanDesk + hora + data
    Header::create();

    // Praia
    lv_obj_t *location = lv_label_create(screen);
    lv_label_set_text(location, "Água de Madeiros");
    lv_obj_set_style_text_font(location, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(location, Theme::color(Theme::Location), 0);
    lv_obj_align(location, LV_ALIGN_TOP_LEFT, 42, 125);

    // Estado atual da maré
    status = lv_label_create(screen);
    lv_obj_set_style_text_font(status, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(status, Theme::color(Theme::TextMuted), 0);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, -55);

    // Próxima baixa-mar
    lv_obj_t *lowLabel = lv_label_create(screen);
    lv_label_set_text(lowLabel, "Próxima baixa-mar");
    lv_obj_set_style_text_font(lowLabel, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(lowLabel, Theme::color(Theme::TextMuted), 0);
    lv_obj_align(lowLabel, LV_ALIGN_CENTER, -220, 35);

    lowTime = lv_label_create(screen);
    lv_obj_set_style_text_font(lowTime, Theme::FontLarge, 0);
    lv_obj_set_style_text_color(lowTime, Theme::color(Theme::TideLow), 0);
    lv_obj_align_to(lowTime, lowLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

    // Próxima preia-mar
    lv_obj_t *highLabel = lv_label_create(screen);
    lv_label_set_text(highLabel, "Próxima preia-mar");
    lv_obj_set_style_text_font(highLabel, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(highLabel, Theme::color(Theme::TextMuted), 0);
    lv_obj_align(highLabel, LV_ALIGN_CENTER, 220, 35);

    highTime = lv_label_create(screen);
    lv_obj_set_style_text_font(highTime, Theme::FontLarge, 0);
    lv_obj_set_style_text_color(highTime, Theme::color(Theme::TideHigh), 0);
    lv_obj_align_to(highTime, highLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

    refresh();
}

void HomeScreen::refresh()
{
    if (status == nullptr || lowTime == nullptr || highTime == nullptr)
    {
        return;
    }

    const TideSnapshot tide = TideService::getSnapshot();
    const char *statusText = "Estado desconhecido";
    if (tide.valid && tide.state == TideState::Rising)
    {
        statusText = "Maré a subir";
    }
    else if (tide.valid && tide.state == TideState::Falling)
    {
        statusText = "Maré a descer";
    }

    lv_label_set_text(status, statusText);
    lv_label_set_text(lowTime, TideService::formatTime(tide.nextLow.timestamp).c_str());
    lv_label_set_text(highTime, TideService::formatTime(tide.nextHigh.timestamp).c_str());
}
