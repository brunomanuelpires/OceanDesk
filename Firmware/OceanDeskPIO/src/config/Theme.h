#pragma once

#include <lvgl.h>
#include "fonts/Fonts.h"

namespace Theme
{
    //=========================================================
    // Colors
    //=========================================================

    // Backgrounds
    static constexpr uint32_t Background   = 0x0B1620;
    static constexpr uint32_t Card         = 0x132430;
    static constexpr uint32_t Border       = 0x20313E;

    // Text
    static constexpr uint32_t Text         = 0xFFFFFF;
    static constexpr uint32_t TextMuted    = 0x8EA6B5;

    // Brand
    static constexpr uint32_t Accent       = 0x55C9F3;
    static constexpr uint32_t Location     = 0x75CDE8;

    // Tides
    static constexpr uint32_t TideLow      = 0x55C9F3;
    static constexpr uint32_t TideHigh     = 0xFFFFFF;

    // Status
    static constexpr uint32_t Success      = 0x38C172;
    static constexpr uint32_t Warning      = 0xF4C542;
    static constexpr uint32_t Danger       = 0xE74C3C;


    //=========================================================
    // Fonts
    //=========================================================

    extern const lv_font_t *FontSmall;
    extern const lv_font_t *FontMedium;
    extern const lv_font_t *FontLarge;
    extern const lv_font_t *FontXL;
    extern const lv_font_t *FontTime;


    //=========================================================
    // Helpers
    //=========================================================

    inline lv_color_t color(uint32_t hex)
    {
        return lv_color_hex(hex);
    }
}
