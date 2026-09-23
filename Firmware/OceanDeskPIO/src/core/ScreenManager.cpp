#include "ScreenManager.h"
#include "screens/HomeScreen.h"
#include "screens/SetupScreen.h"
#include "screens/SettingsScreen.h"
#include <lvgl.h>

namespace { bool homeVisible = false; }

void ScreenManager::showHome()
{
    homeVisible = true;
    HomeScreen::create();
}

void ScreenManager::showSetup()
{
    homeVisible = false;
    SetupScreen::create();
}

void ScreenManager::showSettings()
{
    SettingsScreen::create();
}

bool ScreenManager::isHomeVisible()
{
    return homeVisible;
}
