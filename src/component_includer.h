#pragma once
#include "config.h"

#if AC_CONTROL != 0
#define AC_CONTROL_INIT create_ac_control_screen
#include "ac_control.h"
#endif

#if CLIMATE_CONTROL != 0
#define CLIMATE_CONTROL_INIT create_climate_control_screen
#include "climate_control.h"
#endif 

#if FAN != 0
#define FAN_INIT create_fan_control_screen
#include "fan.h"
#endif 