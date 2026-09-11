#include "HomeScreen.h"
#include <lvgl.h>
#include "config/Theme.h"
#include "../../services/TideService.h"
#include "../../services/ConfigService.h"
#include "HomeScreenLayout.h"

namespace
{
HomeScreenLayout *activeLayout = nullptr;
}

void HomeScreen::create()
{
    lv_obj_t *screen = lv_scr_act();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, Theme::color(Theme::Background), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    activeLayout = &homeScreenLayoutFor(ConfigService::get().displayStyle);
    activeLayout->create(screen);
    refresh();
}

void HomeScreen::refresh()
{
    if (activeLayout != nullptr)
    {
        activeLayout->refresh(TideService::getSnapshot());
    }
}
