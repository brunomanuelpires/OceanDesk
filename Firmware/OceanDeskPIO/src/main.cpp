#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static Board *board = nullptr;

static void button_event_cb(lv_event_t *event)
{
    lv_obj_t *label =
        static_cast<lv_obj_t *>(lv_event_get_user_data(event));

    lv_label_set_text(label, "Touch OK!");
}

void setup()
{
    Serial.begin(115200);
    delay(5000);

    Serial.println();
    Serial.println("OceanDesk PlatformIO");
    Serial.println("Initializing board...");

    board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();

    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();

    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(
            lcd->getFrameWidth() * 10
        );
    }
#endif
#endif

    if (!board->begin()) {
        Serial.println("ERROR: Board initialization failed.");
        return;
    }

    Serial.println("Initializing LVGL...");

    if (!lvgl_port_init(board->getLCD(), board->getTouch())) {
        Serial.println("ERROR: LVGL initialization failed.");
        return;
    }

    Serial.println("Creating OceanDesk UI...");

    lvgl_port_lock(-1);

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x08141F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "OceanDesk");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 40, 30);

    lv_obj_t *location = lv_label_create(screen);
    lv_label_set_text(location, "Agua de Madeiros");
    lv_obj_set_style_text_font(location, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(location, lv_color_hex(0x8FA9BB), 0);
    lv_obj_align_to(
        location,
        title,
        LV_ALIGN_OUT_BOTTOM_LEFT,
        0,
        8
    );

    lv_obj_t *low_tide_label = lv_label_create(screen);
    lv_label_set_text(low_tide_label, "Proxima baixa-mar");
    lv_obj_set_style_text_font(
        low_tide_label,
        &lv_font_montserrat_20,
        0
    );
    lv_obj_set_style_text_color(
        low_tide_label,
        lv_color_hex(0x8FA9BB),
        0
    );
    lv_obj_align(low_tide_label, LV_ALIGN_CENTER, -220, -60);

    lv_obj_t *low_tide_time = lv_label_create(screen);
    lv_label_set_text(low_tide_time, "12:45");
    lv_obj_set_style_text_font(
        low_tide_time,
        &lv_font_montserrat_48,
        0
    );
    lv_obj_set_style_text_color(
        low_tide_time,
        lv_color_hex(0x4FC3F7),
        0
    );
    lv_obj_align_to(
        low_tide_time,
        low_tide_label,
        LV_ALIGN_OUT_BOTTOM_MID,
        0,
        15
    );

    lv_obj_t *high_tide_label = lv_label_create(screen);
    lv_label_set_text(high_tide_label, "Proxima preia-mar");
    lv_obj_set_style_text_font(
        high_tide_label,
        &lv_font_montserrat_20,
        0
    );
    lv_obj_set_style_text_color(
        high_tide_label,
        lv_color_hex(0x8FA9BB),
        0
    );
    lv_obj_align(high_tide_label, LV_ALIGN_CENTER, 220, -60);

    lv_obj_t *high_tide_time = lv_label_create(screen);
    lv_label_set_text(high_tide_time, "18:32");
    lv_obj_set_style_text_font(
        high_tide_time,
        &lv_font_montserrat_48,
        0
    );
    lv_obj_set_style_text_color(
        high_tide_time,
        lv_color_hex(0xFFFFFF),
        0
    );
    lv_obj_align_to(
        high_tide_time,
        high_tide_label,
        LV_ALIGN_OUT_BOTTOM_MID,
        0,
        15
    );

    lv_obj_t *button = lv_btn_create(screen);
    lv_obj_set_size(button, 220, 70);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -45);

    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "TESTAR TOUCH");
    lv_obj_center(button_label);

    lv_obj_add_event_cb(
        button,
        button_event_cb,
        LV_EVENT_CLICKED,
        button_label
    );

    lvgl_port_unlock();

    Serial.println("OceanDesk UI ready.");
}

void loop()
{
    delay(1000);
}