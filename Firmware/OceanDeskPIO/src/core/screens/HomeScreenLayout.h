#pragma once

#include <lvgl.h>
#include "../../models/TideData.h"
#include "../../services/ConfigService.h"

class HomeScreenLayout
{
public:
    virtual ~HomeScreenLayout() = default;
    virtual void create(lv_obj_t *screen) = 0;
    // The clock is the only UI element that normally changes every minute.
    // Keep it separate from the tide/graph update to avoid a full RGB redraw.
    virtual void refreshClock() = 0;
    virtual void refresh(const TideSnapshot &snapshot) = 0;
};

HomeScreenLayout &homeScreenLayoutFor(DisplayStyle style);
