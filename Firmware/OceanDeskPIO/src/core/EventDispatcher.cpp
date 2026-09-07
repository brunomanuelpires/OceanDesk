#include "EventDispatcher.h"
#include "screens/HomeScreen.h"
#include "widgets/Header.h"

void EventDispatcher::begin()
{
    // Reservado para inicialização futura do sistema de eventos.
}

void EventDispatcher::refresh()
{
    Header::refresh();
    HomeScreen::refresh();
}
