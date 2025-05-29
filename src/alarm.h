#ifndef ALARM_H
#define ALARM_H

#include <lvgl.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_alarm_screen();
void alarm_set_state(String alarm_state_, String alarm_lock_);

#ifdef __cplusplus
} // extern "C"

void alarm_set_state(StaticJsonDocument<256>& doc);
#endif

#endif // ALARM_H
