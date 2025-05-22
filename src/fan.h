#ifndef FAN_H
#define FAN_H

#include <lvgl.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_fan_control_screen();
void fan_set_state(bool light_power_, bool fan_dir_, unsigned long timer_epoch_, int16_t current_level_);

#ifdef __cplusplus
} // extern "C"

void fan_set_state(StaticJsonDocument<256>& doc);
#endif

#endif // FAN_H
