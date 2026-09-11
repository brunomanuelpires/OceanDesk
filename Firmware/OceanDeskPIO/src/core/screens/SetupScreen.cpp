#include "SetupScreen.h"

#include <lvgl.h>
#include "config/Theme.h"
#include "services/WiFiService.h"

namespace
{
void styleText(lv_obj_t *label, const lv_font_t *font, uint32_t color)
{
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, Theme::color(color), 0);
}

lv_obj_t *createStep(lv_obj_t *screen, int number, const char *text, lv_coord_t y)
{
    lv_obj_t *badge = lv_obj_create(screen);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 38, 38);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, Theme::color(Theme::Accent), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_align(badge, LV_ALIGN_TOP_LEFT, 175, y);

    lv_obj_t *numberLabel = lv_label_create(badge);
    lv_label_set_text_fmt(numberLabel, "%d", number);
    styleText(numberLabel, Theme::FontMedium, Theme::Background);
    lv_obj_center(numberLabel);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, text);
    styleText(label, Theme::FontMedium, Theme::Text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 235, y + 6);
    return label;
}
}

void SetupScreen::create()
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, Theme::color(Theme::Background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *brand = lv_label_create(screen);
    lv_label_set_text(brand, "OceanDesk");
    styleText(brand, Theme::FontMedium, Theme::Accent);
    lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 32);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Vamos configurar o teu OceanDesk");
    styleText(title, Theme::FontLarge, Theme::Text);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 82);

    lv_obj_t *subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "Usa o telemóvel ou o computador para concluir estes passos.");
    styleText(subtitle, Theme::FontSmall, Theme::TextMuted);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 130);

    createStep(screen, 1, "Abre as definições de Wi-Fi", 185);

    lv_obj_t *step2 = createStep(screen, 2, "Liga-te à rede", 245);
    lv_obj_t *network = lv_label_create(screen);
    lv_label_set_text(network, WiFiService::setupNetworkName());
    styleText(network, Theme::FontMedium, Theme::Accent);
    lv_obj_align_to(network, step2, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    lv_obj_t *step3 = createStep(screen, 3, "Abre o navegador e acede a", 305);
    lv_obj_t *address = lv_label_create(screen);
    lv_label_set_text(address, WiFiService::setupAddress());
    styleText(address, Theme::FontMedium, Theme::Accent);
    lv_obj_align_to(address, step3, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    createStep(screen, 4, "Segue a página de configuração", 365);

    lv_obj_t *waiting = lv_label_create(screen);
    lv_label_set_text(waiting, "A aguardar configuração…");
    styleText(waiting, Theme::FontSmall, Theme::TextMuted);
    lv_obj_align(waiting, LV_ALIGN_BOTTOM_MID, 0, -45);
}
