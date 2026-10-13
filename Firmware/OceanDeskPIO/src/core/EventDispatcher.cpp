#include "EventDispatcher.h"
#include "screens/HomeScreen.h"
#include "../services/BrightnessService.h"
#include "ScreenManager.h"
#include <time.h>

void EventDispatcher::begin()
{
    // Reservado para inicialização futura do sistema de eventos.
}

void EventDispatcher::refresh(bool tideSnapshotChanged)
{
    if (ScreenManager::isHomeVisible())
    {
        // RTC and NTP both update the system clock. A minute change should
        // touch only the two header labels; redrawing the tide view here
        // invalidates a large RGB area and can disturb the display pipeline.
        static time_t displayedMinute = -1;
        const time_t currentMinute = time(nullptr) / 60;
        const bool minuteChanged = currentMinute != displayedMinute;
        displayedMinute = currentMinute;

        // Tide calculations and the graph/events are refreshed only after a
        // new snapshot has actually been produced (currently every 15 min).
        // A full tide refresh also updates the header, so do not redraw it
        // twice when it happens exactly on a minute boundary.
        if (tideSnapshotChanged)
        {
            HomeScreen::refresh();
        }
        else if (minuteChanged)
        {
            HomeScreen::refreshClock();
        }
        HomeScreen::update();
    }
    BrightnessService::update();
}
