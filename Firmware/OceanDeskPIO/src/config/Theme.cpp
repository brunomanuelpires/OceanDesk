#include "Theme.h"

namespace Theme
{
    uint32_t Background = 0x0B1620;
    uint32_t Text = 0xFFFFFF;
    uint32_t TextSecondary = 0xB6C7D1;
    uint32_t TextTertiary = 0x8196A4;
    const lv_font_t *FontSmall  = &montserrat_14_pt;
    const lv_font_t *FontMedium = &montserrat_20_pt;
    const lv_font_t *FontLarge  = &montserrat_30_pt;
    const lv_font_t *FontXL     = &montserrat_48_pt;
    const lv_font_t *FontTime   = &montserrat_128_time;
    const lv_font_t *FontClock  = &montserrat_128_bold;
    const lv_font_t *FontHeight = &montserrat_96_bold;
    const lv_font_t *FontSmallStrong = &montserrat_18_bold;
    const lv_font_t *FontMediumStrong = &montserrat_20_medium;
    const lv_font_t *FontLargeStrong = &montserrat_30_medium;
    const lv_font_t *FontHeaderLocation = &montserrat_44_medium;
    const lv_font_t *FontHeaderBrand = &montserrat_24_medium;
    const lv_font_t *FontHeaderDate = &montserrat_21_medium;

    void configure(uint32_t primaryTextColor, uint32_t secondaryTextColor,
                   uint32_t tertiaryTextColor, uint32_t backgroundColor,
                   BackgroundSource)
    {
        Text = primaryTextColor;
        TextSecondary = secondaryTextColor;
        TextTertiary = tertiaryTextColor;
        Background = backgroundColor;
    }
}
