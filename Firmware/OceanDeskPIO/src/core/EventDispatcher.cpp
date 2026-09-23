#include "EventDispatcher.h"
#include "screens/HomeScreen.h"
#include "widgets/Header.h"
#include "../services/BrightnessService.h"
#include "ScreenManager.h"

void EventDispatcher::begin()
{
    // Reservado para inicialização futura do sistema de eventos.
}

void EventDispatcher::refresh()
{
    if (ScreenManager::isHomeVisible())
    {
        Header::refresh();
        HomeScreen::refresh();
        HomeScreen::update();
    }
    BrightnessService::update();
}
