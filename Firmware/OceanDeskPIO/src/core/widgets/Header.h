#pragma once

#include <lvgl.h>

class Header
{
public:
    static void create();
    static void refresh();

private:
    static lv_obj_t *timeLabel;
    static lv_obj_t *dateLabel;

    static void timerCallback(lv_timer_t *timer);
};