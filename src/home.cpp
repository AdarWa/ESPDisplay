#include <lvgl.h>
#include "home.h"
#include "config.h"
#include "component_includer.h"
#include "font_styles.h"
#include "battery_monitor.h"

lv_obj_t *create_app_button(lv_obj_t *parent, const char *name,const lv_font_t *font, const char *component) {
    // Create the button
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 80, 80);  // Adjust size as needed

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_add_style(label, &fa_style,0);
    lv_obj_center(label);

    // Add transition event to load new screen
    if(component == "AC_CONTROL"){
      #ifdef AC_CONTROL_INIT
      lv_label_set_text(label, ICONS_AC);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {lv_scr_load_anim(AC_CONTROL_INIT(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);}, LV_EVENT_CLICKED, NULL);
      #endif
    }else if(component == "CLIMATE_CONTROL"){
      #ifdef CLIMATE_CONTROL_INIT
      lv_label_set_text(label, ICONS_TEMPERATURE);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {lv_scr_load_anim(CLIMATE_CONTROL_INIT(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);}, LV_EVENT_CLICKED, NULL);
      #endif
    }else if(component == "FAN"){
      #ifdef FAN_INIT
      lv_label_set_text(label, ICONS_FAN);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {lv_scr_load_anim(FAN_INIT(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);}, LV_EVENT_CLICKED, NULL);
      #endif
    }else if(component == "ALARM"){
      #ifdef ALARM_INIT
      lv_label_set_text(label, ICONS_SIREN);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {lv_scr_load_anim(ALARM_INIT(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);}, LV_EVENT_CLICKED, NULL);
      #endif
    }else if(component == "ROBOROCK"){
      #ifdef ROBOROCK_INIT
      lv_label_set_text(label, ICONS_ROOMBA);
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {lv_scr_load_anim(ROBOROCK_INIT(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);}, LV_EVENT_CLICKED, NULL);
      #endif
    }
    
    // Create a small text label below the icon
    lv_obj_t *text_label = lv_label_create(btn);
    lv_label_set_text(text_label, name);
    lv_obj_set_style_text_font(text_label, font, 0);
    set_black_text(text_label);
    lv_obj_align(text_label, LV_ALIGN_BOTTOM_MID, 0, 0); // Adjust Y offset as needed

    return btn;
}

lv_obj_t *create_home_screen() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf5f5f5), 0);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE); // Don't scroll the root

    // Fixed Status Bar
    lv_obj_t *status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, LV_PCT(100), 30);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Battery Percentage Label
    lv_obj_t *battery_label = lv_label_create(status_bar);
    lv_label_set_text_fmt(battery_label, "%d%%", (int)battery_get_level());
    lv_obj_align(battery_label, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_12, 0);
    if(battery_get_level() < ALERT_BATTERY_LEVEL) {
        lv_obj_set_style_text_color(battery_label, lv_color_hex(COLORS_RED), 0);
    }

    // Scrollable Content Container
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, LV_PCT(100), LV_VER_RES - 30); // Full height minus status bar
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0); // Align to bottom to sit below status bar
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    // Welcome Text
    lv_obj_t *welcome_label = lv_label_create(content);
    lv_label_set_text(welcome_label, WELCOME_MSG);
    lv_obj_align(welcome_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(welcome_label, &lv_font_montserrat_28, 0);

    // Container for "apps"
    lv_obj_t *apps_cont = lv_obj_create(content);
    lv_obj_set_size(apps_cont, LV_PCT(100), LV_SIZE_CONTENT); // Auto height
    lv_obj_set_flex_flow(apps_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(apps_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(apps_cont, 10, 0);
    lv_obj_set_style_bg_opa(apps_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(apps_cont, 0, 0);
    lv_obj_align(apps_cont, LV_ALIGN_TOP_MID, 0, 50); // Below welcome label
    #if AC_CONTROL != 0
      create_app_button(apps_cont, "AC",&lv_font_montserrat_14, "AC_CONTROL");
    #endif

    #if CLIMATE_CONTROL != 0
      create_app_button(apps_cont, "Climate",&lv_font_montserrat_14, "CLIMATE_CONTROL");      
    #endif

    #if FAN != 0
      create_app_button(apps_cont, "Fan",&lv_font_montserrat_14, "FAN");
    #endif

    #if ALARM != 0
      create_app_button(apps_cont, "Alarm",&lv_font_montserrat_14, "ALARM");
    #endif

    #if ROBOROCK != 0
      create_app_button(apps_cont, "Roborock",&lv_font_montserrat_14, "ROBOROCK");
    #endif

    lv_obj_set_scroll_dir(apps_cont, LV_DIR_NONE);          // Disable scrolling on the container
    // lv_obj_set_size(apps_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    return scr;
}