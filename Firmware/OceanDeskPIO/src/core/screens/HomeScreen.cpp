#include "HomeScreen.h"
#include <lvgl.h>
#include <cmath>
#include <time.h>
#include "fonts/Fonts.h"
#include "config/Theme.h"
#include "../widgets/Header.h"
#include "../../services/TideService.h"

namespace
{
lv_obj_t *status = nullptr;
lv_obj_t *height = nullptr;
lv_obj_t *lowTime = nullptr;
lv_obj_t *lowHeight = nullptr;
lv_obj_t *highTime = nullptr;
lv_obj_t *highHeight = nullptr;
lv_obj_t *tideCurve = nullptr;
lv_obj_t *tideMarker = nullptr;

constexpr size_t tideCurvePointCount = 33;
constexpr lv_coord_t tideCurveLeft = 20;
constexpr lv_coord_t tideCurveTop = 15;
constexpr lv_coord_t tideCurveWidth = 260;
constexpr lv_coord_t tideCurveHeight = 95;
constexpr double pi = 3.14159265358979323846;
lv_point_t tideCurvePoints[tideCurvePointCount];

void setEventHeight(lv_obj_t *label, const TideEvent &event)
{
    if (!event.valid)
    {
        lv_label_set_text(label, "--");
        return;
    }

    char text[20];
    snprintf(text, sizeof(text), "%.1f m ZH", event.height);
    lv_label_set_text(label, text);
}

double calculateTideProgress(const TideSnapshot &tide)
{
    if (!tide.valid || !tide.previousEvent.valid)
    {
        return 0.0;
    }

    const TideEvent &nextEvent = tide.state == TideState::Rising
                                     ? tide.nextHigh
                                     : tide.nextLow;
    const time_t now = time(nullptr);
    const time_t duration = nextEvent.timestamp - tide.previousEvent.timestamp;
    if (!nextEvent.valid || duration <= 0)
    {
        return 0.0;
    }

    double progress = static_cast<double>(now - tide.previousEvent.timestamp) /
                      static_cast<double>(duration);
    if (progress < 0.0)
    {
        progress = 0.0;
    }
    else if (progress > 1.0)
    {
        progress = 1.0;
    }

    return progress;
}

void updateTideCurve(const TideSnapshot &tide)
{
    if (!tide.valid || tide.state == TideState::Unknown)
    {
        lv_obj_add_flag(tideCurve, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(tideMarker, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const bool rising = tide.state == TideState::Rising;
    for (size_t index = 0; index < tideCurvePointCount; ++index)
    {
        const double progress = static_cast<double>(index) /
                                static_cast<double>(tideCurvePointCount - 1);
        const double easedHeight = 0.5 - 0.5 * std::cos(pi * progress);
        tideCurvePoints[index].x = tideCurveLeft + static_cast<lv_coord_t>(
            progress * tideCurveWidth + 0.5);
        tideCurvePoints[index].y = tideCurveTop + static_cast<lv_coord_t>(
            (rising ? 1.0 - easedHeight : easedHeight) * tideCurveHeight + 0.5);
    }

    lv_line_set_points(tideCurve, tideCurvePoints, tideCurvePointCount);

    const double progress = calculateTideProgress(tide);
    const double easedHeight = 0.5 - 0.5 * std::cos(pi * progress);
    const lv_coord_t markerX = tideCurveLeft + static_cast<lv_coord_t>(
        progress * tideCurveWidth + 0.5);
    const lv_coord_t markerY = tideCurveTop + static_cast<lv_coord_t>(
        (rising ? 1.0 - easedHeight : easedHeight) * tideCurveHeight + 0.5);

    lv_obj_set_pos(tideMarker, markerX - 9, markerY - 9);
    lv_obj_clear_flag(tideCurve, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tideMarker, LV_OBJ_FLAG_HIDDEN);
}
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
    lv_obj_set_style_text_color(location, Theme::color(Theme::Text), 0);
    lv_obj_align(location, LV_ALIGN_TOP_RIGHT, -22, 18);

    // Estado atual da maré
    status = lv_label_create(screen);
    lv_obj_set_style_text_font(status, Theme::FontLarge, 0);
    lv_obj_set_style_text_color(status, Theme::color(Theme::Text), 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 270);

    height = lv_label_create(screen);
    lv_obj_set_style_text_font(height, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(height, Theme::color(Theme::Text), 0);
    lv_obj_align(height, LV_ALIGN_TOP_MID, 0, 318);

    // Próxima baixa-mar
    lv_obj_t *lowLabel = lv_label_create(screen);
    lv_label_set_text(lowLabel, "Próxima baixa-mar");
    lv_obj_set_style_text_font(lowLabel, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(lowLabel, Theme::color(Theme::Text), 0);
    lv_obj_align(lowLabel, LV_ALIGN_TOP_LEFT, 55, 420);

    lowTime = lv_label_create(screen);
    lv_obj_set_style_text_font(lowTime, Theme::FontLarge, 0);
    lv_obj_set_style_text_color(lowTime, Theme::color(Theme::Text), 0);
    lv_obj_align_to(lowTime, lowLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lowHeight = lv_label_create(screen);
    lv_obj_set_style_text_font(lowHeight, Theme::FontSmall, 0);
    lv_obj_set_style_text_color(lowHeight, Theme::color(Theme::Text), 0);
    lv_obj_align_to(lowHeight, lowTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    // Próxima preia-mar
    lv_obj_t *highLabel = lv_label_create(screen);
    lv_label_set_text(highLabel, "Próxima preia-mar");
    lv_obj_set_style_text_font(highLabel, Theme::FontMedium, 0);
    lv_obj_set_style_text_color(highLabel, Theme::color(Theme::Text), 0);
    lv_obj_align(highLabel, LV_ALIGN_TOP_RIGHT, -55, 420);

    highTime = lv_label_create(screen);
    lv_obj_set_style_text_font(highTime, Theme::FontLarge, 0);
    lv_obj_set_style_text_color(highTime, Theme::color(Theme::Text), 0);
    lv_obj_align_to(highTime, highLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    highHeight = lv_label_create(screen);
    lv_obj_set_style_text_font(highHeight, Theme::FontSmall, 0);
    lv_obj_set_style_text_color(highHeight, Theme::color(Theme::Text), 0);
    lv_obj_align_to(highHeight, highTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    // Curva do ciclo atual: sobe ou desce entre o último e o próximo extremo.
    lv_obj_t *graph = lv_obj_create(screen);
    lv_obj_remove_style_all(graph);
    lv_obj_set_size(graph, 300, 130);
    lv_obj_align(graph, LV_ALIGN_TOP_MID, 0, 405);
    lv_obj_clear_flag(graph, LV_OBJ_FLAG_SCROLLABLE);

    tideCurve = lv_line_create(graph);
    lv_obj_set_style_line_width(tideCurve, 8, 0);
    lv_obj_set_style_line_color(tideCurve, Theme::color(Theme::Text), 0);
    lv_obj_set_style_line_rounded(tideCurve, true, 0);

    tideMarker = lv_obj_create(graph);
    lv_obj_remove_style_all(tideMarker);
    lv_obj_set_size(tideMarker, 18, 18);
    lv_obj_set_style_radius(tideMarker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tideMarker, Theme::color(Theme::Accent), 0);
    lv_obj_set_style_bg_opa(tideMarker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tideMarker, Theme::color(Theme::Text), 0);
    lv_obj_set_style_border_width(tideMarker, 3, 0);
    lv_obj_set_style_border_opa(tideMarker, LV_OPA_COVER, 0);

    refresh();
}

void HomeScreen::refresh()
{
    if (status == nullptr || height == nullptr ||
        lowTime == nullptr || lowHeight == nullptr ||
        highTime == nullptr || highHeight == nullptr ||
        tideCurve == nullptr || tideMarker == nullptr)
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
    if (tide.valid)
    {
        char heightText[40];
        snprintf(heightText, sizeof(heightText), "Altura prevista: %.1f m ZH", tide.currentHeight);
        lv_label_set_text(height, heightText);
    }
    else
    {
        lv_label_set_text(height, "Altura prevista: --");
    }
    lv_label_set_text(lowTime, TideService::formatTime(tide.nextLow.timestamp).c_str());
    lv_label_set_text(highTime, TideService::formatTime(tide.nextHigh.timestamp).c_str());
    setEventHeight(lowHeight, tide.nextLow);
    setEventHeight(highHeight, tide.nextHigh);
    updateTideCurve(tide);
}
