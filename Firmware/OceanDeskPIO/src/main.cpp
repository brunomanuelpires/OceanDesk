#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "core/OceanDesk.h"
#include "ui/Dashboard.h"
#include "core/ScreenManager.h"
#include "services/ClockService.h"
#include "lvgl_v8_port.h"
#include "core/widgets/Header.h"
#include "services/WiFiService.h"
#include "services/NTPService.h"
#include "core/EventDispatcher.h"

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

    // Inicializa o relógio
    ClockService::begin();

    // Inicializa o Wi-Fi
    WiFiService::begin();

    NTPService::begin();

    Serial.println("Creating OceanDesk UI...");

    lvgl_port_lock(-1);

    EventDispatcher::begin();
    ScreenManager::showHome();

    lvgl_port_unlock();

    Serial.println("OceanDesk UI ready.");
}

void loop()
{
    WiFiService::update();
    ClockService::update();
    NTPService::update();

    lvgl_port_lock(-1);
    EventDispatcher::refresh();
    lvgl_port_unlock();

    delay(1000);
}