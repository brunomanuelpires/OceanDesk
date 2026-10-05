#include "EventDispatcher.h"
#include "screens/HomeScreen.h"
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
        // The displayed clock has minute precision and tide predictions only
        // change materially over minutes. Redrawing the complete home view on
        // every service tick caused a periodic full visual workload on the RGB
        // pipeline, despite most labels retaining identical text.
        static unsigned long lastVisualRefresh = 0;
        const unsigned long now = millis();
        if (lastVisualRefresh == 0 || now - lastVisualRefresh >= 60UL * 1000UL)
        {
            lastVisualRefresh = now;
            HomeScreen::refresh();
        }
        HomeScreen::update();
    }
    BrightnessService::update();
}
