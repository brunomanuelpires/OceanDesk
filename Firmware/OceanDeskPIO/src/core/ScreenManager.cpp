#include "ScreenManager.h"
#include "screens/HomeScreen.h"
#include "screens/SetupScreen.h"

void ScreenManager::showHome()
{
    HomeScreen::create();
}

void ScreenManager::showSetup()
{
    SetupScreen::create();
}
