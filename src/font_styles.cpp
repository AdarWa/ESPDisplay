#include "font_styles.h"
#include "fa_font.h"
#include "config.h"


lv_style_t fa_style;

void init_styles() {
    lv_style_init(&fa_style);
    lv_style_set_text_font(&fa_style, &fa_font);
    lv_style_set_text_color(&fa_style, lv_color_hex(COLORS_BLACK));
}
