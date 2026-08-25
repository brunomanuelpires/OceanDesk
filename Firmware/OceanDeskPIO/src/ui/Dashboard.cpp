#include "Dashboard.h"
#include <lvgl.h>
#include "fonts/Fonts.h"

void Dashboard::create()
{
    // Fundo
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Título
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "OceanDesk");
    lv_obj_set_style_text_font(title, &montserrat_30_pt, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Praia
    lv_obj_t *beach = lv_label_create(lv_scr_act());
    lv_label_set_text(beach, "Água de Madeiros");
    lv_obj_set_style_text_font(beach, &montserrat_20_pt, 0);
    lv_obj_set_style_text_color(beach, lv_color_hex(0x80CBC4), 0);
    lv_obj_align(beach, LV_ALIGN_TOP_MID, 0, 70);

    // Hora
    lv_obj_t *clock = lv_label_create(lv_scr_act());
    lv_label_set_text(clock, "15:42");
    lv_obj_set_style_text_font(clock, &montserrat_48_pt, 0);
    lv_obj_set_style_text_color(clock, lv_color_white(), 0);
    lv_obj_align(clock, LV_ALIGN_CENTER, 0, -40);

    // Maré
    lv_obj_t *tide = lv_label_create(lv_scr_act());
    lv_label_set_text(tide, "Maré Alta\n18:46");
    lv_obj_set_style_text_font(tide, &montserrat_20_pt, 0);
    lv_obj_set_style_text_color(tide, lv_color_hex(0x64B5F6), 0);
    lv_obj_align(tide, LV_ALIGN_CENTER, 0, 60);
}