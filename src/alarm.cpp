#include "alarm.h"
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
static String alarm_state = "disarmed";
static String alarm_lock = "unlocked";
static unsigned long last_update = 0;
static bool is_locked = false;
static const char* scr_name = "alarm_screen";

static lv_obj_t *status_label = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t* buttons[2] = {NULL, NULL};

#define MAX_CODE_LEN 4
static char code_buffer[MAX_CODE_LEN + 1] = "";
#define is_code_avail() (strlen(code_buffer) == MAX_CODE_LEN)
static lv_obj_t *code_label;

// an enum to represent the pending action
#define PENDING_ACTION_NONE 0
#define PENDING_ACTION_HOME 1
#define PENDING_ACTION_DISARM 2
static uint16_t pending_action = PENDING_ACTION_NONE;

#define pending_action_to_str(action) \
    ((action) == PENDING_ACTION_HOME ? "arm_home" : \
     (action) == PENDING_ACTION_DISARM ? "disarm" : \
     "none")



// Helper function to get a friendly name for the alarm state instead of uncapitalized strings with underscores
static String get_friendly_alarm_state() {
    if (alarm_state == "armed_home") return "Armed Home";
    if (alarm_state == "armed_away") return "Armed Away";
    if (alarm_state == "armed_vacation") return "Armed Vacation";
    if (alarm_state == "disarmed") return "Disarmed";
    if (alarm_state == "arming") return "Arming";
    return "Unknown";
}

static const char* get_short_alarm_state() {
    if (alarm_state == "armed_home") return "Home";
    if (alarm_state == "armed_away") return "Away";
    if (alarm_state == "armed_vacation") return "Vacation";
    if (alarm_state == "disarmed") return "Disarm";
    if (alarm_state == "arming") return "Arming";
    return "Unknown";
}

static void update_mqtt(const char* action, const char* state, bool raw = false, const char* code = NULL){
    // TODO: Implement working MQTT logic
    if (raw) {
        HAMqtt* mqtt = getMqtt();
        mqtt->publish(MQTT_ALARM_TOPIC, action);
        return;
    }
    JsonDocument doc;
    doc["action"] = action;
    doc["state"] = state;
    if(code != NULL){
        doc["code"] = code;
    }
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
static void update_arming_slider();

static void update_scr(){
    update_slider_state(false);
    update_arming_slider();
    update_alarm_state();
    update_last_update_time();
}

void alarm_set_state(String alarm_state_, String alarm_lock_, unsigned long last_update_){
    alarm_state = alarm_state_;
    alarm_lock = alarm_lock_;
    last_update = last_update_;
    is_locked = (alarm_lock == "locked" || alarm_lock == "locking");
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
static lv_obj_t *arming_slider_btn;

static void update_slider_state(bool send) {
    LV_LOG_USER("Updating slider state: %s", is_locked ? "Locked" : "Unlocked");
    if (is_locked) {
        lv_coord_t max_x = 120; // TODO: Make this dynamic based on the container size. on first init the container isn't initilized yet, so we use a fixed value as temporary fix.
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


static void update_code_display() {
    static char masked[MAX_CODE_LEN + 1];
    int len = strlen(code_buffer);
    for (int i = 0; i < len; ++i)
        masked[i] = '*';
    masked[len] = '\0';
    lv_label_set_text(code_label, masked);
}

static void digit_btn_event_handler(lv_event_t *e) {
    const char *digit = (const char*)lv_event_get_user_data(e);
    if (strlen(code_buffer) < MAX_CODE_LEN) {
        strcat(code_buffer, digit);
        update_code_display();
    }
}

static void erase_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    code_buffer[0] = '\0';
    update_code_display();
}

static void confirm_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    printf("Code entered: %s\n", code_buffer);
    if(!is_code_avail()) {
        create_warning_label(lv_scr_act(), "Invalid code entered!");
        return;
    }
    lv_scr_load(create_alarm_screen());
}

void create_code_entry_screen() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    // Label to show masked input
    code_label = lv_label_create(scr);
    lv_obj_align(code_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_label_set_text(code_label, "");
    lv_obj_set_style_text_font(code_label, &lv_font_montserrat_24, 0);

    // Create responsive grid container
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_size(grid, LV_PCT(100), LV_PCT(90));
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    static lv_coord_t col_dsc[] = {LV_PCT(33), LV_PCT(33), LV_PCT(34), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_PCT(20), LV_PCT(20), LV_PCT(20), LV_PCT(20), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    const char *digits[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

    // Create digit buttons
    for (int i = 0; i < 9; ++i) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1,
                                  LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_add_event_cb(btn, digit_btn_event_handler, LV_EVENT_CLICKED, (void *)digits[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, digits[i]);
        lv_obj_center(label);
        set_black_text(label);
    }

    // "0" button
    lv_obj_t *btn0 = lv_btn_create(grid);
    lv_obj_set_grid_cell(btn0, LV_GRID_ALIGN_STRETCH, 1, 1,
                               LV_GRID_ALIGN_STRETCH, 3, 1);
    lv_obj_add_event_cb(btn0, digit_btn_event_handler, LV_EVENT_CLICKED, (void *)digits[9]);
    lv_obj_set_style_bg_color(btn0, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label0 = lv_label_create(btn0);
    lv_label_set_text(label0, "0");
    lv_obj_center(label0);
    set_black_text(label0);

    // Erase button (Red)
    lv_obj_t *erase_btn = lv_btn_create(grid);
    lv_obj_set_grid_cell(erase_btn, LV_GRID_ALIGN_STRETCH, 0, 1,
                                  LV_GRID_ALIGN_STRETCH, 3, 1);
    lv_obj_add_event_cb(erase_btn, erase_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(erase_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_bg_opa(erase_btn, LV_OPA_COVER, 0);
    lv_obj_t *erase_label = lv_label_create(erase_btn);
    lv_label_set_text(erase_label, "Erase");
    lv_obj_center(erase_label);
    set_black_text(erase_label);

    // Confirm button (Green)
    lv_obj_t *confirm_btn = lv_btn_create(grid);
    lv_obj_set_grid_cell(confirm_btn, LV_GRID_ALIGN_STRETCH, 2, 1,
                                    LV_GRID_ALIGN_STRETCH, 3, 1);
    lv_obj_add_event_cb(confirm_btn, confirm_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0x44CC44), 0);
    lv_obj_set_style_bg_opa(confirm_btn, LV_OPA_COVER, 0);
    lv_obj_t *confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "OK");
    lv_obj_center(confirm_label);
    set_black_text(confirm_label);

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

static void update_arming_slider(){
    lv_obj_t *label = lv_obj_get_child(arming_slider_btn, 0); // Assuming the label is the first child
    if (alarm_state != "disarmed") {
        lv_coord_t max_x = 105; // TODO: Make this dynamic based on the container size. on first init the container isn't initilized yet, so we use a fixed value as a temporary fix.
        lv_label_set_text(label, get_short_alarm_state());
        lv_obj_set_x(arming_slider_btn, max_x);
    } else {
        lv_label_set_text(label, "Disarm");
        lv_obj_set_x(arming_slider_btn, 0);
    }
}

static void slider_event_arming_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t *container = lv_obj_get_parent(btn);
    lv_obj_t *label = lv_obj_get_child(btn, 0); // Assuming the label is the first child

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
            // update_mqtt("arm", "arm_home");
            if(alarm_state != "armed_home") {
                pending_action = PENDING_ACTION_HOME;
                create_code_entry_screen();
            }
        } else {
            // Snap to left
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, btn);
            lv_anim_set_values(&a, x, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
            // update_mqtt("arm", "disarm");
            if(alarm_state != "disarmed") {
                pending_action = PENDING_ACTION_DISARM;
                create_code_entry_screen();
            }
        }
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
    lv_obj_set_size(btn_container, LV_PCT(85), 100);
    lv_obj_set_style_radius(btn_container, 20, 0);
    lv_obj_set_style_bg_color(btn_container, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_pad_all(btn_container, 3, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_align(btn_container, LV_ALIGN_CENTER, 0, -8);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);


    arming_slider_btn = lv_btn_create(btn_container);
    lv_obj_set_size(arming_slider_btn, LV_PCT(50), LV_PCT(100));
    lv_obj_set_style_radius(arming_slider_btn, 20, 0);
    lv_obj_set_style_bg_color(arming_slider_btn, lv_color_hex(0x999999), 0);
    lv_obj_set_style_border_width(arming_slider_btn, 0, 0);
    lv_obj_clear_flag(arming_slider_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(arming_slider_btn, slider_event_arming_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(arming_slider_btn, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_pos(arming_slider_btn, 0, 0);

    lv_obj_t *arming_slider_label = lv_label_create(arming_slider_btn);
    lv_label_set_text(arming_slider_label, "Disarm");
    set_black_text(arming_slider_label);
    lv_obj_set_style_text_font(arming_slider_label, &lv_font_montserrat_24, 0);
    lv_obj_center(arming_slider_label);


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
    fetch_state();
    if(pending_action != PENDING_ACTION_NONE && is_code_avail()) {
        update_mqtt("arm", pending_action_to_str(pending_action),false, code_buffer); // Send the pending action with the code via MQTT to HA
        code_buffer[0] = '\0'; // Clear the code buffer after sending
    }
    return scr;
}