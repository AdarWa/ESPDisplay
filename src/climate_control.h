#ifndef CLIMATE_CONTROL_H
#define CLIMATE_CONTROL_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* create_climate_control_screen();
void climate_set_state(bool power_, int16_t current_temp_);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CLIMATE_CONTROL_H
