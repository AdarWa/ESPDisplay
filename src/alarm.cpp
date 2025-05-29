#include "alarm.h"
#include "home.h"
#include "config.h"
#include "ha_helper.h"
#include "utils.h"
#include <lvgl.h>
#include "spiffs_handler.h"


static bool first_init = true;
static String alarm_state = "disarmed";
static String alarm_lock = "unlocked";
static unsigned long last_update = 0;
static bool is_locked = false;
static const char* scr_name = "alarm_screen";

static lv_obj_t *status_label = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t* buttons[2] = {NULL, NULL};


// Helper function to get a friendly name for the alarm state instead of uncapitalized strings with underscores
static String get_friendly_alarm_state() {
    if (alarm_state == "armed_home") return "Armed Home";
    if (alarm_state == "armed_away") return "Armed Away";
    if (alarm_state == "armed_vacation") return "Armed Vacation";
    if (alarm_state == "disarmed") return "Disarmed";
    return "Unknown";
}

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
        char time_str[64]; // FIXME: might cause buffer overflow
        simplifyTimeDiff(diff, time_str, sizeof(time_str));
        lv_label_set_text(time_label, time_str);
    } else {
        lv_label_set_text(time_label, "Never");
    }
}

static void update_alarm_state(){
    lv_label_set_text(status_label, get_friendly_alarm_state().c_str());
    for (int i = 0; i < 2; i++) {
        if (buttons[i] != NULL) {
            const char *label = (const char *)lv_obj_get_user_data(buttons[i]);
            if (strcmp(label, get_friendly_alarm_state().c_str()) == 0) {
                lv_obj_set_style_bg_color(buttons[i], lv_color_hex(0x999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_bg_color(buttons[i], lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }
}

static void update_slider_state(bool send = true);

static void update_scr(){
    update_slider_state(false);
    update_alarm_state();
    update_last_update_time();
}

void alarm_set_state(String alarm_state_, String alarm_lock_, unsigned long last_update_){
    alarm_state = alarm_state_;
    alarm_lock = alarm_lock_;
    last_update = last_update_;
    is_locked = (alarm_lock == "locked");
    if(lv_obj_get_user_data(lv_scr_act()) == (void*)(scr_name)){
        update_scr();
    }
}



void alarm_set_state(StaticJsonDocument<256>& doc){
    alarm_set_state(doc.containsKey("alarm_state") ? doc["alarm_state"].as<String>() : alarm_state,
        doc.containsKey("alarm_lock") ? doc["alarm_lock"].as<String>() : alarm_lock,
        doc.containsKey("alarm_last_update") ? doc["alarm_last_update"].as<unsigned long>() : last_update);
}

static void fetch_state(){
    update_mqtt("fetch", "", true);
}

static lv_obj_t *slider_btn;
static lv_obj_t *slider_label;

static void update_slider_state(bool send) {
    if (is_locked) {
        lv_coord_t btn_w = lv_obj_get_width(slider_btn);
        lv_obj_t *container = lv_obj_get_parent(slider_btn);
        lv_coord_t container_w = lv_obj_get_width(container);
        lv_coord_t max_x = container_w - btn_w;
        lv_label_set_text(slider_label, "Locked");
        lv_obj_set_x(slider_btn, max_x);
        lv_obj_set_style_bg_color(slider_btn, lv_color_hex(0x4CAF50), 0);          // Button green
        lv_obj_set_style_bg_color(lv_obj_get_parent(slider_btn), lv_color_hex(0xC8E6C9), 0); // Light green
    } else {
        lv_label_set_text(slider_label, "Unlocked");
        lv_obj_set_x(slider_btn, 0);
        lv_obj_set_style_bg_color(slider_btn, lv_color_hex(0xF44336), 0);          // Button red
        lv_obj_set_style_bg_color(lv_obj_get_parent(slider_btn), lv_color_hex(0x3A1E1E), 0); // Light red
    }
    if(send)
        update_mqtt("lock", is_locked ? "Lock" : "Unlock");
}

static void slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t *container = lv_obj_get_parent(btn);

    lv_coord_t btn_w = lv_obj_get_width(btn);
    lv_coord_t container_w = lv_obj_get_width(container);
    lv_coord_t max_x = container_w - btn_w;

    if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t vect;
        lv_indev_get_vect(indev, &vect);
        lv_coord_t new_x = lv_obj_get_x(btn) + vect.x;

        if (new_x < 0) new_x = 0;
        if (new_x > max_x) new_x = max_x;

        lv_obj_set_pos(btn, new_x, 0);  // y = 0 for perfect vertical alignment
    }

    else if (code == LV_EVENT_RELEASED) {
        lv_coord_t x = lv_obj_get_x(btn);
        if (x > max_x / 2) {
            // Snap to right
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, btn);
            lv_anim_set_values(&a, x, max_x);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
            lv_label_set_text(slider_label, "Locked");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);          // Button green
            lv_obj_set_style_bg_color(container, lv_color_hex(0xC8E6C9), 0); // Light green
            is_locked = true;
        } else {
            // Snap to left
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, btn);
            lv_anim_set_values(&a, x, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
            lv_label_set_text(slider_label, "Unlocked");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xF44336), 0);          // Button red
            lv_obj_set_style_bg_color(container, lv_color_hex(0x3A1E1E), 0); // Light red
            is_locked = false;
        }
        update_mqtt("lock", is_locked ? "Lock" : "Unlock");
    }
}


void create_arming_screen(lv_obj_t *parent) {
    // Background
    lv_obj_set_style_bg_color(parent, lv_color_white(), 0);

    // Status label
    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, get_friendly_alarm_state().c_str());
    lv_obj_set_style_text_color(status_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 8);

    // Time label
    time_label = lv_label_create(parent);
    update_last_update_time();
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(time_label, status_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    // Button container (slightly smaller)
    lv_obj_t *btn_container = lv_obj_create(parent);
    lv_obj_set_size(btn_container, 130, 200);
    lv_obj_set_style_radius(btn_container, 20, 0);
    lv_obj_set_style_bg_color(btn_container, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_pad_all(btn_container, 3, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_align(btn_container, LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_layout(btn_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const char *labels[] = { "Home", "Disarm" };
    static lv_color_t highlight_color = lv_color_hex(0x999999);
    static lv_color_t reg_color = lv_color_hex(0x444444);


    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(btn_container);
        lv_obj_set_size(btn, 85, 85);  // Smaller button
        lv_obj_set_style_radius(btn, 18, 0);
        lv_obj_set_style_bg_color(btn, strcmp(labels[i], get_friendly_alarm_state().c_str()) == 0 ? highlight_color : reg_color, 0);
        lv_obj_set_user_data(btn, (void*)(labels[i])); // Store label text in user data
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_text_color(btn, lv_color_white(), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_18, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
    }

    lv_obj_t *slider_container = lv_obj_create(parent);
    lv_obj_set_size(slider_container, 200, 44);
    lv_obj_set_style_radius(slider_container, 22, 0);
    lv_obj_set_style_bg_color(slider_container, lv_color_hex(0x3A1E1E), 0);
    lv_obj_set_style_border_width(slider_container, 0, 0);
    lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(slider_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_clear_flag(slider_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(slider_container, 0, 0);
    lv_obj_set_style_pad_row(slider_container, 0, 0);
    lv_obj_set_style_pad_column(slider_container, 0, 0);


    // Draggable button (the "slider")
    slider_btn = lv_btn_create(slider_container);
    lv_obj_set_size(slider_btn, 80, 44);
    lv_obj_set_style_radius(slider_btn, 22, 0);
    lv_obj_set_style_bg_color(slider_btn, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_border_width(slider_btn, 0, 0);
    lv_obj_clear_flag(slider_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(slider_btn, slider_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(slider_btn, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_pos(slider_btn, 0, 0);

    // Label inside slider
    slider_label = lv_label_create(slider_btn);
    lv_label_set_text(slider_label, "Unlocked");
    lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(slider_label, &lv_font_montserrat_12, 0);
    lv_obj_center(slider_label);

}


static void back_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_home_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
}

lv_obj_t* create_alarm_screen() {
    fetch_state();
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

    create_arming_screen(scr);
    return scr;
}