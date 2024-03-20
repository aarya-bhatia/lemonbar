#include "module.h"
#include <stdlib.h>

#define LEFT "%{l}"
#define CENTER "%{c}"
#define RIGHT "%{r}"

void setup()
{
    add_module("scripts/active_window.sh", LEFT, UPDATE_PERSIST, 0);
    add_module("python3 scripts/bspwm.py", CENTER, UPDATE_PERSIST, 0);
    add_module("scripts/volume.sh", RIGHT, UPDATE_SIGNAL, 0);
    add_module("scripts/battery.sh", NULL, UPDATE_INTERVAL, 10);
    add_module("scripts/date.sh", NULL, UPDATE_INTERVAL, 10);
}
