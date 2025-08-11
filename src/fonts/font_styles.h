#pragma once
#include <lvgl.h>

// lightbulb, air-conditioner, temperature, fan, siren, roomba
//Font icon definitions
#define ICONS_LIGHTBULB "\xEF\x83\xAB"
#define ICONS_AC "\xEF\xA3\xB4"
#define ICONS_TEMPERATURE "\xEF\x8B\x8A"
#define ICONS_FAN "\xEF\xA1\xA3"
#define ICONS_SIREN "\xEE\x80\xAE"
#define ICONS_ROOMBA "\xEE\x81\x8E"

extern lv_style_t fa_style;

void init_styles();