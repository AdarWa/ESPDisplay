#include "ac_control.h"
#include "home.h"
#include "config.h"
#include "ha_helper.h"
#include "utils.h"
#include "climate_control.h"
#include <lvgl.h>
#include "spiffs_handler.h"


static int16_t current_temp = DEFAULT_TEMP;
static lv_obj_t *temp_label;
static lv_obj_t *btn_power;
static bool ac_power = true;

static bool first_init = true;

static int16_t current_fan_level = 3;
static lv_obj_t *fan_dots[FAN_LEVELS];



static void save_state(){
    StaticJsonDocument<256> state;
    state["ac_power"] = ac_power;
    state["current_temp"] = current_temp;
    state["current_fan_level"] = current_fan_level;
    spiffs_save_json("/ac_control.json", state);
}

static void update_mqtt(bool save = true){
    if(save){
        save_state();
    }
    ac_control_power.setState(ac_power);
    ac_control_temp.setValue((float)current_temp);
    ac_control_fan.setValue((float)current_fan_level+1);
}

void ac_set_state(bool ac_power_, int16_t current_temp_, int16_t current_fan_level_){
    ac_power = ac_power_;
    current_temp = current_temp_;
    current_fan_level = current_fan_level_;
    update_mqtt();
}

void ac_set_state(StaticJsonDocument<256>& doc){
    ac_set_state(doc.containsKey("ac_control_power") ? doc["ac_control_power"] : ac_power,
        doc.containsKey("ac_control_temp") ? doc["ac_control_temp"] : current_temp,
        doc.containsKey("ac_control_fan") ? doc["ac_control_fan"] : current_fan_level);
}

static void fetch_state(){
    if(!first_init){
        return;
    }
    first_init = false;
    if(!spiffs_file_exists("/ac_control.json")){
        Serial.println("ac_control state file not found, creating default state");
        save_state();
    }
    Serial.println("Loading ac_control state from file");
    StaticJsonDocument<256> state;
    if(spiffs_load_json("/ac_control.json", state)){
        ac_power = state["ac_power"];
        current_temp = state["current_temp"];
        current_fan_level = state["current_fan_level"];
    }else{
        Serial.println("Failed to load ac_control state from file");
    }
    update_mqtt(false);
}


static void update_fan_bar_display() {
    for (int i = 0; i < FAN_LEVELS; i++) {
        if (i <= current_fan_level) {
            lv_obj_set_style_bg_color(fan_dots[i], lv_color_hex(i == FAN_LEVELS-1 ? COLORS_ORANGE : COLORS_LIGHT_BLUE), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(fan_dots[i], lv_color_hex(COLORS_GRAY), LV_PART_MAIN);
        }
    }
    update_mqtt();
}

static lv_obj_t* create_fan_bar(lv_obj_t *parent) {
    lv_obj_t *fan_bar = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(fan_bar, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(fan_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Let the size be automatically calculated from content
    lv_obj_set_size(fan_bar, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // Enable flex layout and center everything
    lv_obj_set_layout(fan_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(fan_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fan_bar,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(fan_bar, 5, 0);
    lv_obj_set_style_pad_column(fan_bar, 3, 0);  // Adjust spacing between dots

    for (int i = 0; i < FAN_LEVELS; i++) {
        fan_dots[i] = lv_obj_create(fan_bar);
        lv_obj_set_size(fan_dots[i], i == FAN_LEVELS-1 ? 18 : 12, i == FAN_LEVELS-1 ? 18 : 12);  // Dot size
        lv_obj_set_style_radius(fan_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(fan_dots[i], lv_color_hex(COLORS_GRAY), LV_PART_MAIN);
        lv_obj_clear_flag(fan_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    update_fan_bar_display();
    return fan_bar;
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
        lv_scr_load_anim(create_ac_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    return scr;
}


static void advanced_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_advanced_screen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);
}

static void back_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_home_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
}

static void btn_fan_level_inc_event_cb(lv_event_t *e) {
    if (current_fan_level < FAN_LEVELS-1) {
        current_fan_level++;
        update_fan_bar_display();
    }
}

static void btn_fan_level_dec_event_cb(lv_event_t *e) {
    if (current_fan_level > 0) {
        current_fan_level--;
        update_fan_bar_display();
    }
}

static void update_temp_display() {
    static char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d°", current_temp);
    lv_label_set_text(temp_label, buf);
    update_mqtt();
}

static void update_power_display() {
    lv_color_t color = ac_power ? lv_color_hex(COLORS_GREEN)
                                : lv_color_hex(COLORS_RED);
    lv_obj_set_style_bg_color(btn_power, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn_power, LV_OPA_COVER, LV_PART_MAIN);
    update_mqtt();
}

static void btn_temp_inc_event_cb(lv_event_t *e) {
    if (current_temp < MAX_TEMP) {
        current_temp++;
        update_temp_display();
    }
}

static void btn_temp_dec_event_cb(lv_event_t *e) {
    if (current_temp > MIN_TEMP) {
        current_temp--;
        update_temp_display();
    }
}

static void btn_power_toggle_cb(lv_event_t *e) {
    ac_power = !ac_power;
    update_power_display();
}

lv_obj_t* create_ac_control_screen() {
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

    // Power icon button (top center)
    btn_power = lv_btn_create(scr);
    lv_obj_set_size(btn_power, 60, 60);
    lv_obj_align(btn_power, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_event_cb(btn_power, btn_power_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_label_create(btn_power);
    lv_label_set_text(icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
    lv_obj_center(icon);
    set_black_text(icon);
    update_power_display();

    // Row for - Temp + controls
    lv_obj_t *control_row = lv_obj_create(scr);
    lv_obj_set_size(control_row, LV_PCT(100), 100);
    lv_obj_set_flex_flow(control_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(control_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(control_row, 5, 0);
    lv_obj_set_layout(control_row, LV_LAYOUT_FLEX);
    lv_obj_align(control_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_align(control_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Minus Button f44336
    lv_obj_t *btn_dec = lv_btn_create(control_row);
    lv_obj_set_size(btn_dec, 60, 60);
    lv_obj_add_event_cb(btn_dec, btn_temp_dec_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_dec = lv_label_create(btn_dec);
    lv_label_set_text(label_dec, "-");
    lv_obj_set_style_text_font(label_dec, &lv_font_montserrat_32, 0);  // Bigger font
    lv_obj_center(label_dec);
    lv_obj_set_style_bg_color(btn_dec, lv_color_hex(COLORS_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
    set_black_text(label_dec);
    // Temperature label
    temp_label = lv_label_create(control_row);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_28, 0);  // Smaller font
    update_temp_display();
    lv_obj_set_style_pad_left(temp_label, 10, 0);
    lv_obj_set_style_pad_right(temp_label, 10, 0);

    // Plus Button 4caf50
    lv_obj_t *btn_inc = lv_btn_create(control_row);
    lv_obj_set_size(btn_inc, 60, 60);
    lv_obj_add_event_cb(btn_inc, btn_temp_inc_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_inc = lv_label_create(btn_inc);
    lv_label_set_text(label_inc, "+");
    lv_obj_set_style_text_font(label_inc, &lv_font_montserrat_32, 0);  // Bigger font
    lv_obj_center(label_inc);
    lv_obj_set_style_bg_color(btn_inc, lv_color_hex(COLORS_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    set_black_text(label_inc);

    // Row for Fan Level controls (smaller buttons)
    lv_obj_t *fan_control_row = lv_obj_create(scr);
    lv_obj_set_size(fan_control_row, LV_PCT(100), 60);  // Smaller row size
    lv_obj_set_flex_flow(fan_control_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(fan_control_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(fan_control_row, 5, 0);  // Smaller padding
    lv_obj_set_layout(fan_control_row, LV_LAYOUT_FLEX);
    lv_obj_align(fan_control_row, LV_ALIGN_CENTER, 0, 110);
    lv_obj_set_flex_align(fan_control_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Minus Button (Decrease Fan Level)
    lv_obj_t *fan_btn_dec = lv_btn_create(fan_control_row);
    lv_obj_set_size(fan_btn_dec, 40, 40);  // Smaller button size
    lv_obj_add_event_cb(fan_btn_dec, btn_fan_level_dec_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *fan_label_dec = lv_label_create(fan_btn_dec);
    lv_label_set_text(fan_label_dec, "-");
    lv_obj_set_style_text_font(fan_label_dec, &lv_font_montserrat_26, 0);  // Bigger font
    lv_obj_center(fan_label_dec);
    lv_obj_set_style_bg_color(fan_btn_dec, lv_color_hex(COLORS_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
    set_black_text(fan_label_dec);


    // Fan Level label (in the center)
    // fan_level_label = lv_label_create(fan_control_row);
    // lv_obj_set_style_text_font(fan_level_label, &lv_font_montserrat_28, 0);  // Smaller font size
    // update_fan_level_display();


    lv_obj_t *fan_bar = create_fan_bar(fan_control_row);
    update_fan_bar_display();
    lv_obj_set_style_pad_left(fan_bar, 10, 0);
    lv_obj_set_style_pad_right(fan_bar, 10, 0);

    // Plus Button (Increase Fan Level)
    lv_obj_t *fan_btn_inc = lv_btn_create(fan_control_row);
    lv_obj_set_size(fan_btn_inc, 40, 40);  // Smaller button size
    lv_obj_add_event_cb(fan_btn_inc, btn_fan_level_inc_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *fan_label_inc = lv_label_create(fan_btn_inc);
    lv_label_set_text(fan_label_inc, "+");
    lv_obj_set_style_text_font(fan_label_inc, &lv_font_montserrat_26, 0);  // Bigger font
    lv_obj_center(fan_label_inc);
    lv_obj_set_style_bg_color(fan_btn_inc, lv_color_hex(COLORS_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    set_black_text(fan_label_inc);
    if(is_climate_enabled())
        create_warning_label(scr, "Controlled by\nClimate Control");
    return scr;
}