#pragma once
#include <stdint.h>

bool battery_init();
float battery_get_level();
float battery_get_voltage();
void battery_update();