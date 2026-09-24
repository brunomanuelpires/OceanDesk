#include <assert.h>
#include <string.h>

#include "core/TimeOfDay.h"

int main()
{
    char value[6];

    assert(formatTimeOfDay(21 * 60 + 30, value, sizeof(value)));
    assert(strcmp(value, "21:30") == 0);
    assert(formatTimeOfDay(7 * 60, value, sizeof(value)));
    assert(strcmp(value, "07:00") == 0);
    assert(!formatTimeOfDay(minutesPerDay, value, sizeof(value)));

    return 0;
}
