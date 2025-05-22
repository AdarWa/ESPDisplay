#ifndef UTILS_H
#define UTILS_H

#include <lvgl.h>
#include <cstring>

void show_message_box(const char* title, const char* message);
void close_message_box();
lv_obj_t* create_warning_label(lv_obj_t* parent, const char* text);

#endif
