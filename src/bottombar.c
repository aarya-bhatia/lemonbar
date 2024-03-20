#include "module.h"
#include <stdlib.h>

#define LEFT "%{l}"
#define CENTER "%{c}"
#define RIGHT "%{r}"

void setup()
{
    add_module("scripts/kernel.sh", LEFT, UPDATE_PERSIST, 0);
    add_module("scripts/uptime.sh", CENTER, UPDATE_INTERVAL, 10);
    add_module("python3 scripts/disk.py", RIGHT, UPDATE_PERSIST, 0);
    add_module("scripts/packages.sh", NULL, UPDATE_INTERVAL, 600);
    add_module("python3 scripts/cpu.py", NULL, UPDATE_PERSIST, 0);
    add_module("python3 scripts/memory.py", NULL, UPDATE_PERSIST, 0);
    add_module("python3 scripts/weather.py", NULL, UPDATE_INTERVAL, 300);
    add_module("scripts/wifi.sh", NULL, UPDATE_INTERVAL, 300);
}
