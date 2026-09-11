#include "HomeScreenLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <esp_heap_caps.h>
#include "../../config/Theme.h"
#include "../../services/ClockService.h"
#include "../../services/TideService.h"
#include "../widgets/Header.h"

namespace
{
constexpr double pi = 3.14159265358979323846;

lv_obj_t *createLabel(lv_obj_t *parent, const char *text, const lv_font_t *font,
                      lv_color_t color, lv_align_t align, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, align, x, y);
    return label;
}

const char *stateText(const TideSnapshot &tide)
{
    if (!tide.valid || tide.state == TideState::Unknown) return "Estado desconhecido";
    return tide.state == TideState::Rising ? "Maré a subir" : "Maré a descer";
}

const char *eventTypeText(TideEventType type, bool includeArticle = false)
{
    if (type == TideEventType::Low) return includeArticle ? "Próxima baixa-mar" : "Baixa-mar";
    if (type == TideEventType::High) return includeArticle ? "Próxima preia-mar" : "Preia-mar";
    return includeArticle ? "Próximo evento" : "Evento";
}

void setCurrentHeight(lv_obj_t *label, const TideSnapshot &tide, bool verbose)
{
    if (!tide.valid)
    {
        lv_label_set_text(label, verbose ? "Altura prevista: --" : "--");
        return;
    }

    char text[40];
    snprintf(text, sizeof(text), verbose ? "Altura prevista: %.1f m ZH" : "%.1f m ZH",
             tide.currentHeight);
    lv_label_set_text(label, text);
}

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

void setEventLine(lv_obj_t *label, const TideEvent &event, bool previous)
{
    if (!event.valid)
    {
        lv_label_set_text(label, previous ? "Último evento  --" : "Próximo evento  --");
        return;
    }

    char text[72];
    snprintf(text, sizeof(text), "%s  %s  %.1f m ZH",
             previous ? (event.type == TideEventType::Low ? "Última baixa-mar" : "Última preia-mar")
                      : eventTypeText(event.type, true),
             TideService::formatTime(event.timestamp).c_str(), event.height);
    lv_label_set_text(label, text);
}

const TideEvent &nextEvent(const TideSnapshot &tide)
{
    if (tide.state == TideState::Rising && tide.nextHigh.valid) return tide.nextHigh;
    if (tide.state == TideState::Falling && tide.nextLow.valid) return tide.nextLow;
    if (!tide.nextLow.valid) return tide.nextHigh;
    if (!tide.nextHigh.valid) return tide.nextLow;
    return tide.nextLow.timestamp < tide.nextHigh.timestamp ? tide.nextLow : tide.nextHigh;
}

double tideProgress(const TideSnapshot &tide)
{
    if (!tide.valid || !tide.previousEvent.valid) return 0.0;
    const TideEvent &next = nextEvent(tide);
    const time_t duration = next.timestamp - tide.previousEvent.timestamp;
    if (!next.valid || duration <= 0) return 0.0;
    double progress = static_cast<double>(time(nullptr) - tide.previousEvent.timestamp) /
                      static_cast<double>(duration);
    if (progress < 0.0) return 0.0;
    if (progress > 1.0) return 1.0;
    return progress;
}

class GraphView
{
public:
    void create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                lv_coord_t width, lv_coord_t height, lv_coord_t lineWidth)
    {
        container = lv_obj_create(parent);
        lv_obj_remove_style_all(container);
        lv_obj_set_pos(container, x, y);
        lv_obj_set_size(container, width, height);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
        viewWidth = width;
        viewHeight = height;
        strokeWidth = lineWidth;

        const size_t bufferSize = static_cast<size_t>(width) * static_cast<size_t>(height);
        alphaBuffer = static_cast<uint8_t *>(heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT));
        if (alphaBuffer != nullptr)
        {
            memset(alphaBuffer, 0, bufferSize);
            imageDescriptor.header.always_zero = 0;
            imageDescriptor.header.w = width;
            imageDescriptor.header.h = height;
            imageDescriptor.header.cf = LV_IMG_CF_ALPHA_8BIT;
            imageDescriptor.data_size = bufferSize;
            imageDescriptor.data = alphaBuffer;
            curve = lv_img_create(container);
            lv_img_set_src(curve, &imageDescriptor);
            lv_obj_set_style_img_recolor(curve, Theme::color(Theme::Text), 0);
            lv_obj_set_style_img_recolor_opa(curve, LV_OPA_COVER, 0);
        }
        else
        {
            curve = lv_line_create(container);
            lv_obj_set_style_line_width(curve, lineWidth, 0);
            lv_obj_set_style_line_color(curve, Theme::color(Theme::Text), 0);
            lv_obj_set_style_line_rounded(curve, true, 0);
        }

        marker = lv_obj_create(container);
        lv_obj_remove_style_all(marker);
        lv_obj_set_size(marker, 18, 18);
        lv_obj_set_style_radius(marker, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(marker, Theme::color(Theme::Accent), 0);
        lv_obj_set_style_bg_opa(marker, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(marker, Theme::color(Theme::Text), 0);
        lv_obj_set_style_border_width(marker, 3, 0);
        lv_obj_set_style_border_opa(marker, LV_OPA_COVER, 0);
    }

    void refresh(const TideSnapshot &tide)
    {
        if (!tide.valid || tide.state == TideState::Unknown)
        {
            lv_obj_add_flag(curve, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        const bool rising = tide.state == TideState::Rising;
        const lv_coord_t left = 18;
        const lv_coord_t top = 14;
        const lv_coord_t width = viewWidth - 36;
        const lv_coord_t height = viewHeight - 28;
        if (alphaBuffer != nullptr)
        {
            if (!curveRendered || renderedRising != rising)
            {
                renderCurve(rising, left, top, width, height);
                renderedRising = rising;
                curveRendered = true;
            }
        }
        else
        {
            for (size_t index = 0; index < fallbackPointCount; ++index)
            {
                const double pointProgress = static_cast<double>(index) / (fallbackPointCount - 1);
                const double pointHeight = 0.5 - 0.5 * std::cos(pi * pointProgress);
                points[index].x = left + static_cast<lv_coord_t>(pointProgress * width + 0.5);
                points[index].y = top + static_cast<lv_coord_t>(
                    (rising ? 1.0 - pointHeight : pointHeight) * height + 0.5);
            }
            lv_line_set_points(curve, points, fallbackPointCount);
        }

        const double progress = tideProgress(tide);
        const double eased = 0.5 - 0.5 * std::cos(pi * progress);
        const lv_coord_t markerX = left + static_cast<lv_coord_t>(progress * width + 0.5);
        const lv_coord_t markerY = top + static_cast<lv_coord_t>((rising ? 1.0 - eased : eased) * height + 0.5);
        lv_obj_set_pos(marker, markerX - 9, markerY - 9);
        lv_obj_clear_flag(curve, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(marker, LV_OBJ_FLAG_HIDDEN);
    }

private:
    void renderCurve(bool rising, double left, double top, double width, double height)
    {
        memset(alphaBuffer, 0, static_cast<size_t>(viewWidth) * viewHeight);
        constexpr size_t segmentCount = 96;
        double previousX = left;
        double previousY = top + (rising ? height : 0.0);
        for (size_t index = 1; index <= segmentCount; ++index)
        {
            const double progress = static_cast<double>(index) / segmentCount;
            const double eased = 0.5 - 0.5 * std::cos(pi * progress);
            const double currentX = left + progress * width;
            const double currentY = top + (rising ? 1.0 - eased : eased) * height;
            drawAntialiasedSegment(previousX, previousY, currentX, currentY);
            previousX = currentX;
            previousY = currentY;
        }
        lv_img_cache_invalidate_src(&imageDescriptor);
        lv_obj_invalidate(curve);
    }

    void drawAntialiasedSegment(double x0, double y0, double x1, double y1)
    {
        const double radius = strokeWidth * 0.5;
        const int minX = std::max(0, static_cast<int>(std::floor(std::min(x0, x1) - radius - 1.0)));
        const int maxX = std::min(static_cast<int>(viewWidth) - 1,
                                  static_cast<int>(std::ceil(std::max(x0, x1) + radius + 1.0)));
        const int minY = std::max(0, static_cast<int>(std::floor(std::min(y0, y1) - radius - 1.0)));
        const int maxY = std::min(static_cast<int>(viewHeight) - 1,
                                  static_cast<int>(std::ceil(std::max(y0, y1) + radius + 1.0)));
        const double dx = x1 - x0;
        const double dy = y1 - y0;
        const double lengthSquared = dx * dx + dy * dy;
        constexpr double edgeWidth = 1.5;

        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                double projection = ((x + 0.5 - x0) * dx + (y + 0.5 - y0) * dy) / lengthSquared;
                if (projection < 0.0) projection = 0.0;
                else if (projection > 1.0) projection = 1.0;
                const double nearestX = x0 + projection * dx;
                const double nearestY = y0 + projection * dy;
                const double distanceX = x + 0.5 - nearestX;
                const double distanceY = y + 0.5 - nearestY;
                const double distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);
                double coverage = (radius + edgeWidth * 0.5 - distance) / edgeWidth;
                if (coverage <= 0.0) continue;
                if (coverage > 1.0) coverage = 1.0;
                const uint8_t opacity = static_cast<uint8_t>(coverage * 255.0 + 0.5);
                uint8_t &pixel = alphaBuffer[static_cast<size_t>(y) * viewWidth + x];
                if (opacity > pixel) pixel = opacity;
            }
        }
    }

    static constexpr size_t fallbackPointCount = 25;
    lv_obj_t *container = nullptr;
    lv_obj_t *curve = nullptr;
    lv_obj_t *marker = nullptr;
    uint8_t *alphaBuffer = nullptr;
    lv_img_dsc_t imageDescriptor = {};
    lv_coord_t viewWidth = 0;
    lv_coord_t viewHeight = 0;
    lv_coord_t strokeWidth = 0;
    bool curveRendered = false;
    bool renderedRising = false;
    lv_point_t points[fallbackPointCount];
};

class WideHeader
{
public:
    void create(lv_obj_t *screen)
    {
        createLabel(screen, "OceanDesk", Theme::FontMedium, Theme::color(Theme::Text),
                    LV_ALIGN_TOP_LEFT, 22, 18);
        createLabel(screen, ConfigService::get().beachName.c_str(), Theme::FontMedium,
                    Theme::color(Theme::Text), LV_ALIGN_TOP_RIGHT, -22, 18);
        clock = createLabel(screen, "", Theme::FontTime, Theme::color(Theme::Text),
                            LV_ALIGN_TOP_MID, 0, 48);
        date = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                           LV_ALIGN_TOP_MID, 0, 158);
    }

    void refresh()
    {
        lv_label_set_text(clock, ClockService::getTime().c_str());
        String dateText = ClockService::getWeekday() + ", " + ClockService::getDate();
        lv_label_set_text(date, dateText.c_str());
        lv_obj_align(clock, LV_ALIGN_TOP_MID, 0, 48);
        lv_obj_align(date, LV_ALIGN_TOP_MID, 0, 158);
    }

private:
    lv_obj_t *clock = nullptr;
    lv_obj_t *date = nullptr;
};

class ClassicLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        Header::create();
        createLabel(screen, ConfigService::get().beachName.c_str(), Theme::FontMedium,
                    Theme::color(Theme::Text), LV_ALIGN_TOP_RIGHT, -22, 18);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 220);
        height = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 260);
        lv_obj_align_to(height, status, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
        previous = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 300);
        lv_obj_align_to(previous, height, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

        const lv_coord_t graphX = (lv_obj_get_width(screen) - 300) / 2;
        graph.create(screen, graphX, 405, 300, 130, 8);
        lowLabel = createLabel(screen, "Próxima baixa-mar", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_LEFT, 55, 420);
        lowTime = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                              LV_ALIGN_TOP_LEFT, 55, 458);
        lv_obj_set_style_text_font(lowTime, Theme::FontLarge, 0);
        lv_obj_align_to(lowTime, lowLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lowHeight = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                                LV_ALIGN_TOP_LEFT, 55, 510);
        lv_obj_align_to(lowHeight, lowTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

        highLabel = createLabel(screen, "Próxima preia-mar", Theme::FontMedium, Theme::color(Theme::Text),
                                LV_ALIGN_TOP_RIGHT, -55, 420);
        highTime = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_RIGHT, -55, 458);
        lv_obj_set_style_text_font(highTime, Theme::FontLarge, 0);
        lv_obj_align_to(highTime, highLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        highHeight = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                                 LV_ALIGN_TOP_RIGHT, -55, 510);
        lv_obj_align_to(highHeight, highTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    }

    void refresh(const TideSnapshot &tide) override
    {
        lv_label_set_text(status, stateText(tide));
        setCurrentHeight(height, tide, true);
        setEventLine(previous, tide.previousEvent, true);
        lv_label_set_text(lowTime, TideService::formatTime(tide.nextLow.timestamp).c_str());
        lv_label_set_text(highTime, TideService::formatTime(tide.nextHigh.timestamp).c_str());
        setEventHeight(lowHeight, tide.nextLow);
        setEventHeight(highHeight, tide.nextHigh);
        lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 220);
        lv_obj_align_to(height, status, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
        lv_obj_align_to(previous, height, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
        lv_obj_align_to(lowTime, lowLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_align_to(lowHeight, lowTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
        lv_obj_align_to(highTime, highLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_align_to(highHeight, highTime, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
        graph.refresh(tide);
    }

private:
    lv_obj_t *status = nullptr;
    lv_obj_t *height = nullptr;
    lv_obj_t *previous = nullptr;
    lv_obj_t *lowLabel = nullptr;
    lv_obj_t *lowTime = nullptr;
    lv_obj_t *lowHeight = nullptr;
    lv_obj_t *highLabel = nullptr;
    lv_obj_t *highTime = nullptr;
    lv_obj_t *highHeight = nullptr;
    GraphView graph;
};

class MinimalLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 252);
        createLabel(screen, "PRÓXIMO EVENTO", Theme::FontMedium, Theme::color(Theme::Text),
                    LV_ALIGN_TOP_MID, 0, 316);
        nextType = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 354);
        nextTime = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 397);
        nextHeight = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::Text),
                                 LV_ALIGN_TOP_MID, 0, 463);
        progress = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 525);
    }

    void refresh(const TideSnapshot &tide) override
    {
        header.refresh();
        lv_label_set_text(status, stateText(tide));
        const TideEvent &next = nextEvent(tide);
        lv_label_set_text(nextType, eventTypeText(next.type, false));
        lv_label_set_text(nextTime, TideService::formatTime(next.timestamp).c_str());
        setEventHeight(nextHeight, next);
        char text[32];
        snprintf(text, sizeof(text), "Ciclo atual: %d%%", static_cast<int>(tideProgress(tide) * 100.0 + 0.5));
        lv_label_set_text(progress, tide.valid ? text : "Ciclo atual: --");
    }

private:
    WideHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *nextType = nullptr;
    lv_obj_t *nextTime = nullptr;
    lv_obj_t *nextHeight = nullptr;
    lv_obj_t *progress = nullptr;
};

class GraphLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 250);
        const lv_coord_t graphWidth = 500;
        const lv_coord_t graphX = (lv_obj_get_width(screen) - graphWidth) / 2;
        graph.create(screen, graphX, 302, graphWidth, 118, 10);
        nextType = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 432);
        nextTime = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 466);
        nextHeight = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                                 LV_ALIGN_TOP_MID, 0, 526);
    }

    void refresh(const TideSnapshot &tide) override
    {
        header.refresh();
        lv_label_set_text(status, stateText(tide));
        graph.refresh(tide);
        const TideEvent &next = nextEvent(tide);
        lv_label_set_text(nextType, eventTypeText(next.type, true));
        lv_label_set_text(nextTime, TideService::formatTime(next.timestamp).c_str());
        setEventHeight(nextHeight, next);
    }

private:
    WideHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *nextType = nullptr;
    lv_obj_t *nextTime = nullptr;
    lv_obj_t *nextHeight = nullptr;
    GraphView graph;
};

class InfoLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, -175, 252);
        height = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 175, 252);
        previous = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 0, 326);

        createLabel(screen, "PRÓXIMA BAIXA-MAR", Theme::FontMedium, Theme::color(Theme::Text),
                    LV_ALIGN_TOP_MID, -250, 410);
        lowTime = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                              LV_ALIGN_TOP_MID, -250, 451);
        lowHeight = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                                LV_ALIGN_TOP_MID, -250, 516);

        createLabel(screen, "PRÓXIMA PREIA-MAR", Theme::FontMedium, Theme::color(Theme::Text),
                    LV_ALIGN_TOP_MID, 250, 410);
        highTime = createLabel(screen, "", Theme::FontXL, Theme::color(Theme::Text),
                               LV_ALIGN_TOP_MID, 250, 451);
        highHeight = createLabel(screen, "", Theme::FontMedium, Theme::color(Theme::Text),
                                 LV_ALIGN_TOP_MID, 250, 516);
    }

    void refresh(const TideSnapshot &tide) override
    {
        header.refresh();
        lv_label_set_text(status, stateText(tide));
        setCurrentHeight(height, tide, false);
        setEventLine(previous, tide.previousEvent, true);
        lv_label_set_text(lowTime, TideService::formatTime(tide.nextLow.timestamp).c_str());
        lv_label_set_text(highTime, TideService::formatTime(tide.nextHigh.timestamp).c_str());
        setEventHeight(lowHeight, tide.nextLow);
        setEventHeight(highHeight, tide.nextHigh);
    }

private:
    WideHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *height = nullptr;
    lv_obj_t *previous = nullptr;
    lv_obj_t *lowTime = nullptr;
    lv_obj_t *lowHeight = nullptr;
    lv_obj_t *highTime = nullptr;
    lv_obj_t *highHeight = nullptr;
};

ClassicLayout classicLayout;
MinimalLayout minimalLayout;
GraphLayout graphLayout;
InfoLayout infoLayout;
}

HomeScreenLayout &homeScreenLayoutFor(DisplayStyle style)
{
    switch (style)
    {
    case DisplayStyle::Minimal: return minimalLayout;
    case DisplayStyle::Graph: return graphLayout;
    case DisplayStyle::Info: return infoLayout;
    case DisplayStyle::Classic: return classicLayout;
    }

    return classicLayout;
}
