#ifndef ROBOROCK_H
#define ROBOROCK_H

#include <lvgl.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_roborock_screen();
void roborock_set_state(String alarm_state_, String alarm_lock_);

#ifdef __cplusplus
} // extern "C"

void roborock_set_state(StaticJsonDocument<256>& doc);
#endif

#endif // ROBOROCK_H
