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
constexpr lv_coord_t footerDividerY = 482;
constexpr lv_coord_t footerLabelY = 508;
constexpr lv_coord_t footerTypeY = 534;
constexpr lv_coord_t footerSummaryY = 562;

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
    snprintf(text, sizeof(text), verbose ? "Altura prevista: %.1f m" : "%.1f m",
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
    snprintf(text, sizeof(text), "%.1f m", event.height);
    lv_label_set_text(label, text);
}

void setEventSummary(lv_obj_t *label, const TideEvent &event)
{
    if (!event.valid)
    {
        lv_label_set_text(label, "--:--  ·  --");
        return;
    }

    char text[32];
    snprintf(text, sizeof(text), "%s  ·  %.1f m",
             TideService::formatTime(event.timestamp).c_str(), event.height);
    lv_label_set_text(label, text);
}

const char *followingEventLabel(const TideEvent &event)
{
    if (!event.valid) return "Evento seguinte";
    return event.type == TideEventType::Low ? "Baixa-mar" : "Preia-mar";
}

void setEventLine(lv_obj_t *label, const TideEvent &event, bool previous)
{
    if (!event.valid)
    {
        lv_label_set_text(label, previous ? "Último evento  --" : "Próximo evento  --");
        return;
    }

    char text[72];
    snprintf(text, sizeof(text), "%s  %s  %.1f m",
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

enum class CurveStyle
{
    SingleCycle,
    MultiCycleForecast
};

class GraphView
{
public:
    void create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                lv_coord_t width, lv_coord_t height, lv_coord_t lineWidth,
                CurveStyle style = CurveStyle::SingleCycle)
    {
        if (alphaBuffer != nullptr)
        {
            heap_caps_free(alphaBuffer);
            alphaBuffer = nullptr;
        }
        curve = nullptr;
        marker = nullptr;
        curveRendered = false;
        container = lv_obj_create(parent);
        lv_obj_remove_style_all(container);
        lv_obj_set_pos(container, x, y);
        lv_obj_set_size(container, width, height);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
        viewWidth = width;
        viewHeight = height;
        viewX = x;
        viewY = y;
        strokeWidth = lineWidth;
        curveStyle = style;

        if (curveStyle == CurveStyle::MultiCycleForecast)
        {
            const size_t bufferSize = static_cast<size_t>(width) * static_cast<size_t>(height);
            alphaBuffer = static_cast<uint8_t *>(heap_caps_malloc(bufferSize,
                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            if (alphaBuffer != nullptr)
            {
                memset(alphaBuffer, 0, bufferSize);
                imageDescriptor.header.always_zero = 0;
                imageDescriptor.header.w = width;
                imageDescriptor.header.h = height;
                imageDescriptor.header.cf = LV_IMG_CF_ALPHA_8BIT;
                imageDescriptor.data_size = bufferSize;
                imageDescriptor.data = alphaBuffer;
                curve = lv_img_create(parent);
                lv_obj_set_pos(curve, x, y);
                lv_img_set_src(curve, &imageDescriptor);
                lv_obj_set_style_img_recolor(curve, Theme::color(Theme::Text), 0);
                lv_obj_set_style_img_recolor_opa(curve, LV_OPA_COVER, 0);
            }
            else
            {
                // A safe low-memory fallback; the normal hardware path uses
                // the antialiased PSRAM image above.
                curve = lv_line_create(parent);
                lv_obj_set_pos(curve, x, y);
                lv_obj_set_size(curve, width, height);
                lv_obj_set_style_line_width(curve, lineWidth, 0);
                lv_obj_set_style_line_color(curve, Theme::color(Theme::Text), 0);
                lv_obj_set_style_line_rounded(curve, true, 0);
                for (lv_obj_t *&dash : forecastDashes)
                {
                    dash = lv_line_create(parent);
                    lv_obj_set_pos(dash, x, y);
                    lv_obj_set_size(dash, width, height);
                    lv_obj_set_style_line_width(dash, lineWidth, 0);
                    lv_obj_set_style_line_color(dash, Theme::color(Theme::Text), 0);
                    lv_obj_set_style_line_rounded(dash, true, 0);
                }
            }
        }
        else
        {
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
                lv_obj_set_size(curve, width, height);
                lv_obj_set_style_line_width(curve, lineWidth, 0);
                lv_obj_set_style_line_color(curve, Theme::color(Theme::Text), 0);
                lv_obj_set_style_line_rounded(curve, true, 0);
            }
        }

        if (curveStyle == CurveStyle::MultiCycleForecast)
        {
            marker = lv_obj_create(parent);
            lv_obj_remove_style_all(marker);
            lv_obj_set_size(marker, 18, 18);
            lv_obj_set_style_radius(marker, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(marker, Theme::color(Theme::Accent), 0);
            lv_obj_set_style_bg_opa(marker, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(marker, Theme::color(Theme::Text), 0);
            lv_obj_set_style_border_width(marker, 3, 0);
            lv_obj_set_style_border_opa(marker, LV_OPA_COVER, 0);
        }
        else
        {
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
    }

    void refresh(const TideSnapshot &tide)
    {
        if (!tide.valid || tide.state == TideState::Unknown)
        {
            curveRendered = false;
            lv_obj_add_flag(curve, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
            for (lv_obj_t *dash : forecastDashes) if (dash != nullptr) lv_obj_add_flag(dash, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        const bool rising = tide.state == TideState::Rising;
        const lv_coord_t left = 18;
        const lv_coord_t top = 14;
        const lv_coord_t width = viewWidth - 36;
        const lv_coord_t height = viewHeight - 28;
        if (curveStyle == CurveStyle::MultiCycleForecast)
        {
            if (!curveRendered)
            {
                if (alphaBuffer != nullptr) renderForecastCurve(left, top, width, height);
                else renderForecastLines(left, top, width, height);
                curveRendered = true;
            }
        }
        else if (alphaBuffer != nullptr)
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
        lv_coord_t markerX;
        lv_coord_t markerY;
        if (curveStyle == CurveStyle::MultiCycleForecast)
        {
            constexpr double startPhase = -0.5;
            constexpr double halfCycles = 4.4;
            const double halfCycleWidth = static_cast<double>(width) / halfCycles;
            const double phase = rising ? progress : 1.0 + progress;
            const double waveHeight = 0.5 - 0.5 * std::cos(pi * phase);
            markerX = left + static_cast<lv_coord_t>((phase - startPhase) * halfCycleWidth + 0.5);
            markerY = top + static_cast<lv_coord_t>((1.0 - waveHeight) * height + 0.5);
        }
        else
        {
            const double eased = 0.5 - 0.5 * std::cos(pi * progress);
            markerX = left + static_cast<lv_coord_t>(progress * width + 0.5);
            markerY = top + static_cast<lv_coord_t>((rising ? 1.0 - eased : eased) * height + 0.5);
        }
        if (curveStyle == CurveStyle::MultiCycleForecast)
        {
            lv_obj_set_pos(marker, viewX + markerX - 9, viewY + markerY - 9);
        }
        else
        {
            lv_obj_set_pos(marker, markerX - 9, markerY - 9);
        }
        lv_obj_clear_flag(curve, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(marker, LV_OBJ_FLAG_HIDDEN);
        for (lv_obj_t *dash : forecastDashes) if (dash != nullptr) lv_obj_clear_flag(dash, LV_OBJ_FLAG_HIDDEN);
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

    void renderForecastCurve(double left, double top, double width, double height)
    {
        memset(alphaBuffer, 0, static_cast<size_t>(viewWidth) * viewHeight);
        constexpr size_t segmentCount = 180;
        constexpr double startPhase = -0.5;
        constexpr double halfCycles = 4.4;
        constexpr double dashedStart = 0.79;
        double previousX = left;
        const double firstHeight = 0.5 - 0.5 * std::cos(pi * startPhase);
        double previousY = top + (1.0 - firstHeight) * height;
        for (size_t index = 1; index <= segmentCount; ++index)
        {
            const double progress = static_cast<double>(index) / segmentCount;
            const double currentX = left + progress * width;
            const double phase = startPhase + progress * halfCycles;
            const double waveHeight = 0.5 - 0.5 * std::cos(pi * phase);
            const double currentY = top + (1.0 - waveHeight) * height;
            const bool solid = progress < dashedStart ||
                               (static_cast<int>((progress - dashedStart) * 40.0) % 2 == 0);
            if (solid) drawAntialiasedSegment(previousX, previousY, currentX, currentY);
            previousX = currentX;
            previousY = currentY;
        }
        lv_img_cache_invalidate_src(&imageDescriptor);
        lv_obj_invalidate(curve);
    }

    void renderForecastLines(double left, double top, double width, double height)
    {
        constexpr double startPhase = -0.5;
        constexpr double halfCycles = 4.4;
        constexpr double dashedStart = 0.79;
        for (size_t index = 0; index < forecastSolidPointCount; ++index)
        {
            const double progress = dashedStart * static_cast<double>(index) / (forecastSolidPointCount - 1);
            const double phase = startPhase + progress * halfCycles;
            const double waveHeight = 0.5 - 0.5 * std::cos(pi * phase);
            forecastSolidPoints[index].x = left + static_cast<lv_coord_t>(progress * width + 0.5);
            forecastSolidPoints[index].y = top + static_cast<lv_coord_t>((1.0 - waveHeight) * height + 0.5);
        }
        lv_line_set_points(curve, forecastSolidPoints, forecastSolidPointCount);

        constexpr size_t pointsPerDash = forecastDashPointCount;
        constexpr double dashLength = 0.026;
        constexpr double dashGap = 0.010;
        for (size_t dashIndex = 0; dashIndex < forecastDashCount; ++dashIndex)
        {
            const double dashStart = dashedStart + dashIndex * (dashLength + dashGap);
            const double dashEnd = std::min(1.0, dashStart + dashLength);
            for (size_t pointIndex = 0; pointIndex < pointsPerDash; ++pointIndex)
            {
                const double progress = dashStart + (dashEnd - dashStart) * pointIndex / (pointsPerDash - 1);
                const double phase = startPhase + progress * halfCycles;
                const double waveHeight = 0.5 - 0.5 * std::cos(pi * phase);
                forecastDashPoints[dashIndex][pointIndex].x = left + static_cast<lv_coord_t>(progress * width + 0.5);
                forecastDashPoints[dashIndex][pointIndex].y = top + static_cast<lv_coord_t>((1.0 - waveHeight) * height + 0.5);
            }
            lv_line_set_points(forecastDashes[dashIndex], forecastDashPoints[dashIndex], pointsPerDash);
        }
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
    static constexpr size_t forecastSolidPointCount = 121;
    static constexpr size_t forecastDashCount = 6;
    static constexpr size_t forecastDashPointCount = 6;
    lv_obj_t *container = nullptr;
    lv_obj_t *curve = nullptr;
    lv_obj_t *marker = nullptr;
    uint8_t *alphaBuffer = nullptr;
    lv_img_dsc_t imageDescriptor = {};
    lv_coord_t viewWidth = 0;
    lv_coord_t viewHeight = 0;
    lv_coord_t viewX = 0;
    lv_coord_t viewY = 0;
    lv_coord_t strokeWidth = 0;
    bool curveRendered = false;
    bool renderedRising = false;
    CurveStyle curveStyle = CurveStyle::SingleCycle;
    lv_point_t points[fallbackPointCount];
    lv_obj_t *forecastDashes[forecastDashCount] = {};
    lv_point_t forecastSolidPoints[forecastSolidPointCount];
    lv_point_t forecastDashPoints[forecastDashCount][forecastDashPointCount];
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

class HomeHeader
{
public:
    void create(lv_obj_t *screen)
    {
        // The two top blocks are deliberately centred on their own halves:
        // time/date on the left and location/product on the right.
        constexpr lv_coord_t leftCenterX = -284;
        constexpr lv_coord_t rightCenterX = 256;
        location = createLabel(screen, ConfigService::get().beachName.c_str(), Theme::FontHeaderLocation,
                               Theme::color(Theme::TextSecondary), LV_ALIGN_TOP_MID, rightCenterX, 28);
        createLabel(screen, "OCEANDESK", Theme::FontHeaderBrand, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, rightCenterX, 82);
        date = createLabel(screen, "", Theme::FontHeaderDate, Theme::color(Theme::TextTertiary),
                           LV_ALIGN_TOP_MID, leftCenterX, 122);
        clock = createLabel(screen, "", Theme::FontClock, Theme::color(Theme::Text),
                            LV_ALIGN_TOP_MID, leftCenterX, 14);
    }

    void refresh()
    {
        lv_label_set_text(clock, ClockService::getTime().c_str());
        const String dateText = ClockService::getWeekday() + ", " + ClockService::getDate();
        lv_label_set_text(date, dateText.c_str());
        lv_obj_align(location, LV_ALIGN_TOP_MID, 256, 28);
        lv_obj_align(clock, LV_ALIGN_TOP_MID, -284, 14);
        lv_obj_align(date, LV_ALIGN_TOP_MID, -284, 122);
    }

private:
    lv_obj_t *date = nullptr;
    lv_obj_t *clock = nullptr;
    lv_obj_t *location = nullptr;
};

void createFooterDivider(lv_obj_t *screen)
{
    lv_obj_t *divider = lv_obj_create(screen);
    lv_obj_remove_style_all(divider);
    lv_obj_set_pos(divider, 36, footerDividerY);
    lv_obj_set_size(divider, lv_disp_get_hor_res(lv_disp_get_default()) - 72, 2);
    lv_obj_set_style_bg_color(divider, Theme::color(Theme::TextSecondary), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_70, 0);
}

class ClassicLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::TextSecondary),
                             LV_ALIGN_TOP_MID, 0, 166);
        height = createLabel(screen, "", Theme::FontHeight, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 205);
        journey = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::TextSecondary),
                              LV_ALIGN_TOP_MID, 0, 306);

        // The screen object has not received its final layout size yet here.
        // Read the display resolution directly so the forecast always spans
        // the physical 1024 px panel instead of collapsing at the left edge.
        const lv_coord_t graphWidth = lv_disp_get_hor_res(lv_disp_get_default()) - 48;
        graph.create(screen, 24, 360, graphWidth, 108, 6,
                     CurveStyle::MultiCycleForecast);
        createFooterDivider(screen);

        createLabel(screen, "ÚLTIMO EVENTO", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, -330, footerLabelY);
        previousType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                                   LV_ALIGN_TOP_MID, -330, footerTypeY);
        previousSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                      LV_ALIGN_TOP_MID, -330, footerSummaryY);

        createLabel(screen, "PRÓXIMO EVENTO", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, 0, footerLabelY);
        nextType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                               LV_ALIGN_TOP_MID, 0, footerTypeY);
        nextSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                  LV_ALIGN_TOP_MID, 0, footerSummaryY);

        createLabel(screen, "EVENTO SEGUINTE", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, 330, footerLabelY);
        followingType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                                    LV_ALIGN_TOP_MID, 330, footerTypeY);
        followingSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                       LV_ALIGN_TOP_MID, 330, footerSummaryY);
    }

    void refreshClock() override
    {
        header.refresh();
    }

    void refresh(const TideSnapshot &tide) override
    {
        refreshClock();
        lv_label_set_text(status, stateText(tide));
        setCurrentHeight(height, tide, false);
        graph.refresh(tide);

        const TideEvent &previous = tide.previousEvent;
        lv_label_set_text(previousType, eventTypeText(previous.type, false));
        setEventSummary(previousSummary, previous);

        const TideEvent &next = nextEvent(tide);
        lv_label_set_text(nextType, eventTypeText(next.type, false));
        setEventSummary(nextSummary, next);

        const TideEvent &following = next.type == TideEventType::Low ? tide.nextHigh : tide.nextLow;
        lv_label_set_text(followingType, followingEventLabel(following));
        setEventSummary(followingSummary, following);

        if (!tide.valid || !next.valid)
        {
            lv_label_set_text(journey, "Previsão indisponível");
        }
        else
        {
            const char *destination = next.type == TideEventType::Low ? "baixa-mar" : "preia-mar";
            char text[48];
            snprintf(text, sizeof(text), "A caminho da %s", destination);
            lv_label_set_text(journey, text);
        }
        lv_obj_align(previousType, LV_ALIGN_TOP_MID, -330, footerTypeY);
        lv_obj_align(previousSummary, LV_ALIGN_TOP_MID, -330, footerSummaryY);
        lv_obj_align(nextType, LV_ALIGN_TOP_MID, 0, footerTypeY);
        lv_obj_align(nextSummary, LV_ALIGN_TOP_MID, 0, footerSummaryY);
        lv_obj_align(followingType, LV_ALIGN_TOP_MID, 330, footerTypeY);
        lv_obj_align(followingSummary, LV_ALIGN_TOP_MID, 330, footerSummaryY);
    }

private:
    HomeHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *height = nullptr;
    lv_obj_t *journey = nullptr;
    lv_obj_t *previousType = nullptr;
    lv_obj_t *previousSummary = nullptr;
    lv_obj_t *nextType = nullptr;
    lv_obj_t *nextSummary = nullptr;
    lv_obj_t *followingType = nullptr;
    lv_obj_t *followingSummary = nullptr;
    GraphView graph;
};

class GraphLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::TextSecondary),
                             LV_ALIGN_TOP_MID, 0, 166);
        currentHeight = createLabel(screen, "", Theme::FontHeight, Theme::color(Theme::Text),
                                    LV_ALIGN_TOP_MID, 0, 205);
        const lv_coord_t graphWidth = lv_disp_get_hor_res(lv_disp_get_default()) - 64;
        graph.create(screen, 32, 315, graphWidth, 140, 6, CurveStyle::MultiCycleForecast);
        createFooterDivider(screen);
        createLabel(screen, "PRÓXIMO EVENTO", Theme::FontSmallStrong,
                    Theme::color(Theme::TextTertiary), LV_ALIGN_TOP_MID, 0, footerLabelY);
        nextType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                               LV_ALIGN_TOP_MID, 0, footerTypeY);
        nextSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                  LV_ALIGN_TOP_MID, 0, footerSummaryY);
    }

    void refreshClock() override
    {
        header.refresh();
    }

    void refresh(const TideSnapshot &tide) override
    {
        refreshClock();
        lv_label_set_text(status, stateText(tide));
        setCurrentHeight(currentHeight, tide, false);
        graph.refresh(tide);
        const TideEvent &next = nextEvent(tide);
        lv_label_set_text(nextType, eventTypeText(next.type, false));
        setEventSummary(nextSummary, next);
        lv_obj_align(nextType, LV_ALIGN_TOP_MID, 0, footerTypeY);
        lv_obj_align(nextSummary, LV_ALIGN_TOP_MID, 0, footerSummaryY);
    }

private:
    HomeHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *currentHeight = nullptr;
    lv_obj_t *nextType = nullptr;
    lv_obj_t *nextSummary = nullptr;
    GraphView graph;
};

class InfoLayout final : public HomeScreenLayout
{
public:
    void create(lv_obj_t *screen) override
    {
        header.create(screen);
        status = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::TextSecondary),
                             LV_ALIGN_TOP_MID, 0, 180);
        height = createLabel(screen, "", Theme::FontHeight, Theme::color(Theme::Text),
                             LV_ALIGN_TOP_MID, 0, 247);
        previous = createLabel(screen, "", Theme::FontLarge, Theme::color(Theme::TextSecondary),
                               LV_ALIGN_TOP_MID, 0, 348);
        createFooterDivider(screen);

        createLabel(screen, "ÚLTIMO EVENTO", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, -330, footerLabelY);
        previousType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                                   LV_ALIGN_TOP_MID, -330, footerTypeY);
        previousSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                      LV_ALIGN_TOP_MID, -330, footerSummaryY);

        createLabel(screen, "PRÓXIMO EVENTO", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, 0, footerLabelY);
        nextType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                               LV_ALIGN_TOP_MID, 0, footerTypeY);
        nextSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                  LV_ALIGN_TOP_MID, 0, footerSummaryY);

        createLabel(screen, "EVENTO SEGUINTE", Theme::FontSmallStrong, Theme::color(Theme::TextTertiary),
                    LV_ALIGN_TOP_MID, 330, footerLabelY);
        followingType = createLabel(screen, "", Theme::FontMediumStrong, Theme::color(Theme::TextSecondary),
                                    LV_ALIGN_TOP_MID, 330, footerTypeY);
        followingSummary = createLabel(screen, "", Theme::FontLargeStrong, Theme::color(Theme::Text),
                                       LV_ALIGN_TOP_MID, 330, footerSummaryY);
    }

    void refreshClock() override
    {
        header.refresh();
    }

    void refresh(const TideSnapshot &tide) override
    {
        refreshClock();
        const TideEvent &next = nextEvent(tide);
        if (next.valid)
        {
            lv_label_set_text(status, eventTypeText(next.type, true));
            lv_label_set_text(height, TideService::formatTime(next.timestamp).c_str());
            char text[32];
            snprintf(text, sizeof(text), "%.1f m", next.height);
            lv_label_set_text(previous, text);
        }
        else
        {
            lv_label_set_text(status, "Próximo evento");
            lv_label_set_text(height, "--:--");
            lv_label_set_text(previous, "Previsão indisponível");
        }
        lv_label_set_text(previousType, eventTypeText(tide.previousEvent.type, false));
        setEventSummary(previousSummary, tide.previousEvent);
        lv_label_set_text(nextType, eventTypeText(next.type, false));
        setEventSummary(nextSummary, next);
        const TideEvent &following = next.type == TideEventType::Low ? tide.nextHigh : tide.nextLow;
        lv_label_set_text(followingType, followingEventLabel(following));
        setEventSummary(followingSummary, following);
        lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 180);
        lv_obj_align(height, LV_ALIGN_TOP_MID, 0, 247);
        lv_obj_align(previous, LV_ALIGN_TOP_MID, 0, 348);
        lv_obj_align(previousType, LV_ALIGN_TOP_MID, -330, footerTypeY);
        lv_obj_align(previousSummary, LV_ALIGN_TOP_MID, -330, footerSummaryY);
        lv_obj_align(nextType, LV_ALIGN_TOP_MID, 0, footerTypeY);
        lv_obj_align(nextSummary, LV_ALIGN_TOP_MID, 0, footerSummaryY);
        lv_obj_align(followingType, LV_ALIGN_TOP_MID, 330, footerTypeY);
        lv_obj_align(followingSummary, LV_ALIGN_TOP_MID, 330, footerSummaryY);
    }

private:
    HomeHeader header;
    lv_obj_t *status = nullptr;
    lv_obj_t *height = nullptr;
    lv_obj_t *previous = nullptr;
    lv_obj_t *previousType = nullptr;
    lv_obj_t *previousSummary = nullptr;
    lv_obj_t *nextType = nullptr;
    lv_obj_t *nextSummary = nullptr;
    lv_obj_t *followingType = nullptr;
    lv_obj_t *followingSummary = nullptr;
};

ClassicLayout classicLayout;
GraphLayout graphLayout;
InfoLayout infoLayout;
}

HomeScreenLayout &homeScreenLayoutFor(DisplayStyle style)
{
    switch (style)
    {
    case DisplayStyle::Graph: return graphLayout;
    case DisplayStyle::Info: return infoLayout;
    case DisplayStyle::Classic: return classicLayout;
    }

    return classicLayout;
}
