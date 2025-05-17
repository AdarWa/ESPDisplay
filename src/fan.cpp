#include "fan.h"
#include "home.h"
#include "config.h"
#include "ha_helper.h"
#include <math.h>


static int16_t current_level = -1;
static lv_obj_t *level_label;
static lv_obj_t *light_btn;
static lv_obj_t *rev_btn;
static bool light_power = false;
static bool fan_dir = false;

static lv_obj_t *fan_btns[6];

static lv_obj_t *hour_roller;
static lv_obj_t *min_roller;

static int16_t timer_min = 0;

static lv_obj_t* create_advanced_screen();

static void update_mqtt(){
    fan_light.setState(light_power);
    fan_reverse.setState(fan_dir);
    fan_speed.setValue((float)current_level+1);
}

static void update_selected_level(){
    for (int i = 0; i < 6; i++) {
        lv_obj_set_style_bg_color(fan_btns[i], lv_color_hex(0xDDDDDD), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(fan_btns[i], LV_OPA_COVER, LV_PART_MAIN);
    }
    if(current_level >= 0) {
        lv_obj_set_style_bg_color(fan_btns[current_level], lv_color_hex(COLORS_LIGHT_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(fan_btns[current_level], LV_OPA_COVER, LV_PART_MAIN);
    }
    update_mqtt();
}

static void time_selector_cb(lv_event_t* e) {
    uint16_t hour = lv_roller_get_selected(hour_roller);
    uint16_t minute = lv_roller_get_selected(min_roller)*5;

    LV_LOG_USER("Selected time: %02d:%02d", hour, minute);
    timer_min = hour * 60 + minute;
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
    lv_roller_set_selected(hour_roller, timer_min/60, LV_ANIM_OFF);
    lv_roller_set_selected(min_roller, (timer_min%60)/5, LV_ANIM_OFF);


    // Add event on minutes roller (or both if needed)
    lv_obj_add_event_cb(min_roller, time_selector_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(hour_roller, time_selector_cb, LV_EVENT_VALUE_CHANGED, nullptr);

}

static lv_obj_t* create_set_timer_screen(){
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xf8f8f8), 0);

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Add an event callback to the button
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
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

    // Add an event callback to the button
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        lv_scr_load_anim(create_fan_control_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, TRANSITION_TIME, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *set_timer_btn = lv_btn_create(scr);
    lv_obj_align(set_timer_btn, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_obj_set_size(set_timer_btn, timer_min > 0 ? 200 : 100, 40);
    lv_obj_set_style_bg_color(set_timer_btn, lv_color_hex(COLORS_LIGHT_BLUE), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *set_timer_label = lv_label_create(set_timer_btn);
    lv_label_set_text(set_timer_label, timer_min > 0 ? "Stop and set timer" : "Set Timer");
    lv_obj_center(set_timer_label);

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

static void update_reverse_display() {
    lv_color_t color = fan_dir ? lv_color_hex(COLORS_GREEN)
                                : lv_color_hex(COLORS_GRAY);
    lv_obj_set_style_bg_color(rev_btn
    , color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rev_btn
    , LV_OPA_COVER, LV_PART_MAIN);

    update_mqtt();
}


static void btn_reverse_toggle_cb(lv_event_t *e) {
    fan_dir = !fan_dir;
    update_mqtt();
}



lv_obj_t* create_fan_control_screen() {
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

    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COLORS_GRAY), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);


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
        if(i < 6){
            fan_btns[i] = btn;
        }
    }
    update_selected_level();

    // Reverse Direction Button (top)
    rev_btn = lv_btn_create(scr);
    lv_obj_set_size(rev_btn, 50, 50);
    lv_obj_align(rev_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(rev_btn, btn_reverse_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *rev_label = lv_label_create(rev_btn);
    lv_label_set_text(rev_label, LV_SYMBOL_REFRESH);  // symbol for reverse
    lv_obj_center(rev_label);
    update_reverse_display();


    // Light Toggle Button (bottom)
    light_btn = lv_btn_create(scr);
    lv_obj_set_size(light_btn, 50, 50);
    lv_obj_align(light_btn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(light_btn, btn_power_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *light_label = lv_label_create(light_btn);
    lv_label_set_text(light_label, LV_SYMBOL_POWER);
    lv_obj_center(light_label);
    update_power_display();
    

    return scr;
}