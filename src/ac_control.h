#ifndef AC_CONTROL_H
#define AC_CONTROL_H

#include <lvgl.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_ac_control_screen();
void ac_set_state(bool ac_power_, int16_t current_temp_, int16_t current_fan_level_);

#ifdef __cplusplus
} // extern "C"

void ac_set_state(StaticJsonDocument<256>& doc);
#endif

#endif // AC_CONTROL_H
