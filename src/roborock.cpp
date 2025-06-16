#include "roborock.h"
#include "home.h"
#include "config.h"
#include "ha_helper.h"
#include "utils.h"
#include <lvgl.h>
#include "spiffs_handler.h"
#include <stdio.h>
#include <string.h>

#define MAX_LAST_UPDATE_DIFF 31556952 // 1 year in seconds, used to check if the last update is valid or is just the unsigned long "reverse"

static bool first_init = true;
static String robo_state = "Idle";
static unsigned long last_update = 0;
static const char* scr_name = "roborock_screen";

static lv_obj_t *time_label = NULL;
static lv_obj_t *status_label = NULL;

static void update_mqtt(const char* action, const char* state, bool raw = false){
    // TODO: Implement working MQTT logic
    if (raw) {
        HAMqtt* mqtt = getMqtt();
        mqtt->publish(MQTT_ALARM_TOPIC, action);
        return;
    }
    JsonDocument doc;
    doc["action"] = action;
    doc["state"] = state;
    HAMqtt* mqtt = getMqtt();
    char out[80]; // FIXME: might cause buffer overflow
    serializeJson(doc, out);
    mqtt->publish(MQTT_ALARM_TOPIC, out);
}

static void update_last_update_time(){
    if (last_update > 0) {
        unsigned long diff = getCurrentEpochTime() - last_update;
        if(diff > 5 && diff < MAX_LAST_UPDATE_DIFF) {
            char time_str[64]; // FIXME: might cause buffer overflow
            simplifyTimeDiff(diff, time_str, sizeof(time_str));
            lv_label_set_text(time_label, strcat(time_str, " ago"));
        }else {
            lv_label_set_text(time_label, "Now");
        }
    } else {
        lv_label_set_text(time_label, "Never");
    }
}

static void update_scr(){
    update_last_update_time();
}

void roborock_set_state(String robo_state_, unsigned long last_update_){
    robo_state = robo_state_;
    last_update = last_update_;
    if(lv_obj_get_user_data(lv_scr_act()) == (void*)(scr_name)){
        update_scr();
    }
}

void roborock_set_state(StaticJsonDocument<256>& doc){
    roborock_set_state(doc.containsKey("roborock_state") ? doc["roborock_state"].as<String>() : robo_state,
        doc.containsKey("roborock_last_update") ? doc["roborock_last_update"].as<unsigned long>() : last_update);
}

static void fetch_state(){
    update_mqtt("fetch", "", true);
}

static void back_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_home_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
}

static void dock_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    update_mqtt("robo_dock", "",true);
    
}

static void stop_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    update_mqtt("robo_stop", "",true);
}

lv_obj_t* create_roborock_screen() {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_user_data(scr, (void*)(scr_name));

    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    set_black_text(back_label);

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, robo_state.c_str());
    lv_obj_set_style_text_color(status_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 8);

    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "...");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(time_label, status_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, LV_PCT(100), 60);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0); // Transparent background
    lv_obj_set_style_border_width(row, 0, 0);       // No border
    lv_obj_set_scroll_dir(row, LV_DIR_NONE);        // No scrolling
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -120); // Align to the bottom center

    // Common style for buttons
    lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 12);
    lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_grad_color(&style_btn, lv_palette_darken(LV_PALETTE_BLUE, 2));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_VER);
    lv_style_set_text_color(&style_btn, lv_color_white());
    lv_style_set_pad_all(&style_btn, 12);
    lv_style_set_border_width(&style_btn, 0);

    // Dock Button
    lv_obj_t *btn_dock = lv_btn_create(row);
    lv_obj_add_style(btn_dock, &style_btn, 0);
    lv_obj_set_size(btn_dock, 100, 40);
    lv_obj_t *label_dock = lv_label_create(btn_dock);
    lv_label_set_text(label_dock, "Dock");
    lv_obj_center(label_dock);

    // Stop Button
    lv_obj_t *btn_stop = lv_btn_create(row);
    lv_obj_add_style(btn_stop, &style_btn, 0);
    lv_obj_set_style_bg_color(btn_stop, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_grad_color(btn_stop, lv_palette_darken(LV_PALETTE_RED, 2), 0);
    lv_obj_set_size(btn_stop, 100, 40);
    lv_obj_t *label_stop = lv_label_create(btn_stop);
    lv_label_set_text(label_stop, "Stop");
    lv_obj_center(label_stop);
    
    lv_obj_add_event_cb(btn_dock, dock_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_stop, stop_btn_event_cb, LV_EVENT_CLICKED, NULL);

    fetch_state();
    return scr;
}