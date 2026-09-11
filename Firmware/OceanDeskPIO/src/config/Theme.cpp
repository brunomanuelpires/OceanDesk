#include "Theme.h"

namespace Theme
{
    uint32_t Background = 0x0B1620;
    uint32_t Text = 0xFFFFFF;
    const lv_font_t *FontSmall  = &montserrat_14_pt;
    const lv_font_t *FontMedium = &montserrat_20_pt;
    const lv_font_t *FontLarge  = &montserrat_30_pt;
    const lv_font_t *FontXL     = &montserrat_48_pt;
    const lv_font_t *FontTime   = &montserrat_128_time;

    void configure(uint32_t textColor, uint32_t backgroundColor)
    {
        Text = textColor;
        Background = backgroundColor;
    }
}
