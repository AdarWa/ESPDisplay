#include "climate_control.h"
#include "home.h"
#include "config.h"
#include <lvgl.h>
#include "ha_helper.h"
#include "spiffs_handler.h"



static int current_temp = DEFAULT_TEMP;
static lv_obj_t *temp_label;
static lv_obj_t *btn_power;
static bool power = true;

static bool first_init = true;

static void save_state(){
    StaticJsonDocument<256> state;
    state["power"] = power;
    state["temp"] = current_temp;
    spiffs_save_json("/climate.json", state);
}

static void update_mqtt(bool save = true){
    if(save){
        save_state();
    }
    climate_enable.setState(power);
    climate_temp.setValue((float)current_temp);
}

void climate_set_state(bool power_, int16_t current_temp_){
    power = power_;
    current_temp = current_temp_;
    update_mqtt();
}

static void fetch_state(){
    if(!first_init){
        return;
    }
    first_init = false;
    if(!spiffs_file_exists("/climate.json")){
        Serial.println("climate state file not found, creating default state");
        save_state();
    }
    Serial.println("Loading climate state from file");
    StaticJsonDocument<256> state;
    if(spiffs_load_json("/climate.json", state)){
        power = state["power"];
        current_temp = state["temp"];
    }else{
        Serial.println("Failed to load climate state from file");
    }
    update_mqtt(false);
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
        lv_scr_load_anim(create_climate_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    return scr;
}


static void advanced_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_advanced_screen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, TRANSITION_TIME, 0, false);
}

static void back_btn_event_cb(lv_event_t *e) {
    lv_scr_load_anim(create_home_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
}

static void update_temp_display() {
    static char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d°", current_temp);
    lv_label_set_text(temp_label, buf);
    update_mqtt();
}

static void update_power_display() {
    lv_color_t color = power ? lv_color_hex(COLORS_GREEN)
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
    power = !power;
    update_power_display();
}

lv_obj_t* create_climate_control_screen() {
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
    update_power_display();
    set_black_text(icon);

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

    return scr;
}