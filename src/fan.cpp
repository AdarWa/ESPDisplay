#include "fan.h"
#include "home.h"
#include "config.h"
#include "ha_helper.h"
#include <math.h>
#include "spiffs_handler.h"
#include "utils.h"
#include "climate_control.h"

#define MAX_TIMER 60*60*23 + 55*60
#define is_timer_running() (timer_epoch - getCurrentEpochTime() < MAX_TIMER)

static int16_t current_level = -1;
static lv_obj_t *level_label;
static lv_obj_t *light_btn;
static lv_obj_t *rev_btn;
static bool light_power = false;
static bool fan_dir = false;

static lv_obj_t *fan_btns[7];

static lv_obj_t *hour_roller;
static lv_obj_t *min_roller;
static lv_obj_t *time_left_label;

static unsigned long timer_epoch = 0;

static bool first_init = true;

static lv_obj_t* create_advanced_screen();

static void save_state(){
    StaticJsonDocument<256> fan_state;
    fan_state["current_level"] = current_level;
    fan_state["light_power"] = light_power;
    fan_state["fan_dir"] = fan_dir;
    fan_state["timer_epoch"] = timer_epoch;
    spiffs_save_json("/fan.json", fan_state);
}

static void update_mqtt(bool save = true){
    if(save){
        save_state();
    }
    fan_light.setState(light_power);
    fan_reverse.setState(fan_dir);
    fan_speed.setValue((float)current_level+1);
    fan_timer.setValue((float)timer_epoch);
}

void fan_set_state(bool light_power_, bool fan_dir_, unsigned long timer_epoch_, int16_t current_level_){
    light_power = light_power_;
    fan_dir = fan_dir_;
    timer_epoch = timer_epoch_;
    current_level = current_level_;
    update_mqtt();
}

void fan_set_state(StaticJsonDocument<256>& doc){
    fan_set_state(doc.containsKey("fan_light") ? doc["fan_light"] : light_power,
        doc.containsKey("fan_reverse") ? doc["fan_reverse"] : fan_dir,
        doc.containsKey("fan_timer") ? doc["fan_timer"] : timer_epoch,
        doc.containsKey("fan_speed") ? doc["fan_speed"] : current_level);
}

static void fetch_state(){
    if(!first_init){
        return;
    }
    first_init = false;
    if(!spiffs_file_exists("/fan.json")){
        Serial.println("Fan state file not found, creating default state");
        save_state();
    }
    Serial.println("Loading fan state from file");
    StaticJsonDocument<256> fan_state;
    if(spiffs_load_json("/fan.json", fan_state)){
        current_level = fan_state["current_level"];
        light_power = fan_state["light_power"];
        fan_dir = fan_state["fan_dir"];
        timer_epoch = fan_state["timer_epoch"];
    }else{
        Serial.println("Failed to load fan state from file");
    }
    update_mqtt(false);
}

static void update_selected_level(){
    for (int i = 0; i < 7; i++) {
        lv_obj_set_style_bg_color(fan_btns[i], lv_color_hex(0xDDDDDD), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(fan_btns[i], LV_OPA_COVER, LV_PART_MAIN);
    }
    int idx = current_level == -1 ? 6 : current_level;
    lv_obj_set_style_bg_color(fan_btns[idx], lv_color_hex(COLORS_GREEN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fan_btns[idx], LV_OPA_COVER, LV_PART_MAIN);
    update_mqtt();
}

static void time_selector_cb(lv_event_t* e) {
    uint16_t hour = lv_roller_get_selected(hour_roller);
    uint16_t minute = lv_roller_get_selected(min_roller)*5;

    LV_LOG_USER("Selected time: %02d:%02d", hour, minute);
    int16_t timer_min = hour * 60 + minute;
    timer_epoch = getCurrentEpochTime() + timer_min * 60;
    update_mqtt();
}

static void create_time_selector(lv_obj_t* parent) {
    // Container to hold both rollers
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 200, 160);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(cont, 10, 0);

    // Hour roller
    hour_roller = lv_roller_create(cont);
    lv_roller_set_options(hour_roller,
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(hour_roller, 3);
    lv_obj_set_width(hour_roller, 60);
    
    // Minute roller
    min_roller = lv_roller_create(cont);
    lv_roller_set_options(min_roller,
        "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(min_roller, 3);
    lv_obj_set_width(min_roller, 60);
    // Set default selected option
    int16_t timer_min = timer_epoch - getCurrentEpochTime();
    if(timer_min < 0 || !is_timer_running()){
        timer_min = 0;
    }
    timer_min = timer_min / 60;
    lv_roller_set_selected(hour_roller, timer_min/60, LV_ANIM_OFF);
    lv_roller_set_selected(min_roller, (timer_min%60)/5, LV_ANIM_OFF);


    // Add event on minutes roller
    lv_obj_add_event_cb(min_roller, time_selector_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(hour_roller, time_selector_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // Save Button
    lv_obj_t* save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 80, 40);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_MID, is_timer_running() ? 50 : 0, -10);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(COLORS_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);
    set_black_text(save_label);

    lv_obj_add_event_cb(save_btn, [](lv_event_t* e) {
        time_selector_cb(e); // Save the selected time
        lv_scr_load_anim(create_fan_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* remove_btn = lv_btn_create(parent);
    lv_obj_set_size(remove_btn, 80, 40);
    lv_obj_align(remove_btn, LV_ALIGN_BOTTOM_MID, -50, -10);
    lv_obj_set_style_bg_color(remove_btn, lv_color_hex(COLORS_RED), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* remove_label = lv_label_create(remove_btn);
    lv_label_set_text(remove_label, "Remove");
    lv_obj_center(remove_label);
    set_black_text(remove_label);

    lv_obj_add_event_cb(remove_btn, [](lv_event_t* e) {
        timer_epoch = getCurrentEpochTime()-1;
        update_mqtt();
        lv_scr_load_anim(create_fan_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);
    if(!is_timer_running()){
        lv_obj_add_flag(remove_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t* create_set_timer_screen(){
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf8f8f8), 0);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    set_black_text(back_label);
    lv_obj_center(back_label);

    // Add an event callback to the button
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        time_selector_cb(e); // Save the selected time
        lv_scr_load_anim(create_advanced_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Set timer:");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    create_time_selector(scr);

    return scr;
}

static lv_obj_t* create_advanced_screen(){
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf8f8f8), 0);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    set_black_text(back_label);

    // Add an event callback to the button
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        lv_scr_load_anim(create_fan_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *set_timer_btn = lv_btn_create(scr);
    lv_obj_align(set_timer_btn, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_obj_set_size(set_timer_btn, (timer_epoch - getCurrentEpochTime()) < MAX_TIMER ? 200 : 100, 40);
    lv_obj_set_style_bg_color(set_timer_btn, lv_color_hex(COLORS_LIGHT_BLUE), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *set_timer_label = lv_label_create(set_timer_btn);
    lv_label_set_text(set_timer_label, (timer_epoch - getCurrentEpochTime()) < MAX_TIMER ? "Stop and set timer" : "Set Timer");
    lv_obj_center(set_timer_label);
    set_black_text(set_timer_label);


    lv_obj_add_event_cb(set_timer_btn, [](lv_event_t *e) {
        lv_scr_load_anim(create_set_timer_screen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);


    return scr;
}



static void advanced_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_advanced_screen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);
}

static void back_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_home_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
}

static void update_power_display() {
    lv_color_t color = light_power ? lv_color_hex(COLORS_GREEN)
                                : lv_color_hex(COLORS_RED);
    lv_obj_set_style_bg_color(light_btn
    , color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(light_btn
    , LV_OPA_COVER, LV_PART_MAIN);
    update_mqtt();
}

static void btn_power_toggle_cb(lv_event_t *e) {
    light_power = !light_power;
    update_power_display();
}

static void btn_reverse_toggle_cb(lv_event_t *e) {
    fan_dir = !fan_dir;
    update_mqtt();
}



lv_obj_t* create_fan_control_screen() {
    fetch_state();
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *btn_advanced = lv_btn_create(scr);
    lv_obj_align(btn_advanced, LV_ALIGN_TOP_RIGHT, 0, 0); // Top-right with padding
    lv_obj_set_size(btn_advanced, 80, 25);
    lv_obj_add_event_cb(btn_advanced, advanced_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_advanced, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Label for button
    lv_obj_t *label_adv = lv_label_create(btn_advanced);
    lv_label_set_text(label_adv, "Advanced");
    lv_obj_center(label_adv);
    set_black_text(label_adv);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    set_black_text(back_label);


    // Fan Speed Buttons (1-6 arranged in a circle)
    const int radius = 70;
    const lv_coord_t cx = 120; // Center X
    const lv_coord_t cy = 120; // Center Y

    for (int i = 0; i < 7; i++) {
        float angle = ((i-1) * (360.0 / 6.0))* (M_PI / 180.0);
        int x = cx + radius * cos(angle);
        int y = cy + radius * sin(angle);

        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_remove_style_all(btn);

        static lv_style_t style_btn;
        lv_style_init(&style_btn);

        lv_style_set_radius(&style_btn, LV_RADIUS_CIRCLE);
        lv_style_set_bg_color(&style_btn, lv_color_hex(0xDDDDDD));  // Light gray
        lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);

        lv_style_set_border_color(&style_btn, lv_color_hex(0x666666)); // Dark gray border
        lv_style_set_border_width(&style_btn, 1);
        lv_style_set_border_opa(&style_btn, LV_OPA_80);

        lv_style_set_outline_width(&style_btn, 0); // No glow
        lv_style_set_shadow_width(&style_btn, 0);  // No shadow

        static lv_style_t style_btn_pressed;
        lv_style_init(&style_btn_pressed);

        // Set a slightly darker background when pressed
        lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xBBBBBB));
        lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);

        // Apply the style
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_size(btn, 40, 40);
        if(i == 6){
            lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
        }else{
            lv_obj_align(btn, LV_ALIGN_CENTER, x - cx, y - cy);
        }
        lv_obj_set_user_data(btn, (void *)(i));

        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
            int index = (int)lv_obj_get_user_data(btn);
            current_level = index;
            if(current_level == 6){
                current_level = -1;
            }
            update_selected_level();
        }, LV_EVENT_CLICKED, NULL);

        lv_obj_t *label = lv_label_create(btn);
        if(i != 6){
            lv_label_set_text_fmt(label, "%d", i + 1);
        }else{
            lv_label_set_text(label, LV_SYMBOL_STOP);
        }
        lv_obj_center(label);
        fan_btns[i] = btn;
    }
    update_selected_level();

    // Reverse Direction Button (top)
    rev_btn = lv_btn_create(scr);
    lv_obj_set_size(rev_btn, 50, 50);
    lv_obj_align(rev_btn, (timer_epoch - getCurrentEpochTime() < MAX_TIMER) ? LV_ALIGN_BOTTOM_LEFT :LV_ALIGN_BOTTOM_MID, (timer_epoch - getCurrentEpochTime() < MAX_TIMER) ? 15 : 0, -10);
    lv_obj_add_event_cb(rev_btn, btn_reverse_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(rev_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *rev_label = lv_label_create(rev_btn);
    lv_label_set_text(rev_label, LV_SYMBOL_REFRESH);  // symbol for reverse
    lv_obj_center(rev_label);
    set_black_text(rev_label);


    // Light Toggle Button
    light_btn = lv_btn_create(scr);
    lv_obj_set_size(light_btn, 50, 50);
    lv_obj_align(light_btn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(light_btn, btn_power_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *light_label = lv_label_create(light_btn);
    lv_label_set_text(light_label, LV_SYMBOL_POWER);
    lv_obj_center(light_label);
    update_power_display();
    set_black_text(light_label);

    time_left_label = lv_label_create(scr);
    lv_obj_set_style_text_font(time_left_label, &lv_font_montserrat_24, 0);
    if(timer_epoch - getCurrentEpochTime() < MAX_TIMER){
        lv_obj_clear_flag(time_left_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(time_left_label, "%02d:%02d:%02d", 
            (timer_epoch - getCurrentEpochTime()) / 3600, 
            ((timer_epoch - getCurrentEpochTime()) % 3600) / 60, 
            (timer_epoch - getCurrentEpochTime()) % 60);
    }else {
        lv_obj_add_flag(time_left_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    lv_obj_align(time_left_label, LV_ALIGN_BOTTOM_MID, 0, -25);

    lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
        if(timer_epoch - getCurrentEpochTime() < MAX_TIMER){
            lv_label_set_text_fmt(time_left_label, "%02d:%02d:%02d", 
                (timer_epoch - getCurrentEpochTime()) / 3600, 
                ((timer_epoch - getCurrentEpochTime()) % 3600) / 60, 
                (timer_epoch - getCurrentEpochTime()) % 60);
        }else {
            lv_obj_set_style_text_font(time_left_label, &lv_font_montserrat_18, 0);
            lv_label_set_text(time_left_label, "Timer Finished");
            lv_timer_del(t);
        }
    }, 1000, NULL);

    if(is_climate_enabled())
        create_warning_label(scr, "Controlled by\nClimate Control");
    

    return scr;
}