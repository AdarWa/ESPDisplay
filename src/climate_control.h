#ifndef CLIMATE_CONTROL_H
#define CLIMATE_CONTROL_H

#include <lvgl.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_climate_control_screen();
void climate_set_state(bool power_, int16_t current_temp_);
bool is_climate_enabled();

#ifdef __cplusplus
} // extern "C"

void climate_set_state(StaticJsonDocument<256>& doc);
#endif

#endif // CLIMATE_CONTROL_H
