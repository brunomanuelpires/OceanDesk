#include "ScreenManager.h"
#include "screens/HomeScreen.h"

void ScreenManager::showHome()
{
    HomeScreen::create();
}