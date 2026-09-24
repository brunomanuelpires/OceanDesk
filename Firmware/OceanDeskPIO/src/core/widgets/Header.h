#pragma once

#include <lvgl.h>

class Header
{
public:
    static void create(lv_obj_t *parent);
    static void clear();
    static void refresh();

private:
    static lv_obj_t *timeLabel;
    static lv_obj_t *dateLabel;
    static lv_obj_t *networkLabel;

    static void timerCallback(lv_timer_t *timer);
};
