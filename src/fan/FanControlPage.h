#include "FanStateModule.h"
#include "base/TimerModule.h"
#include <lvgl.h>
#include "font_styles.h"
#include "utils.h"

static FanStateModule fan_state;
static TimerModule fan_timer(FanStateModule::MAX_TIMER);

static lv_obj_t* screen;
static lv_obj_t* time_left_label;
static lv_obj_t* light_btn;
static lv_obj_t* fan_btns[7];
static lv_obj_t* rev_btn;

// Forward declarations
static void update_selected_level();
static void update_power_display();
static void update_timer_display();

static void on_light_btn_clicked(lv_event_t* e) {
    fan_state.set_light_power(!fan_state.get_light_power());
    update_power_display();
}

static void on_rev_btn_clicked(lv_event_t* e) {
    fan_state.set_fan_dir(!fan_state.get_fan_dir());
    // No visual toggle here, but you could add one
}

static void on_fan_speed_btn_clicked(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (index == 6) index = -1;  // stop symbol

    fan_state.set_current_level(index);
    update_selected_level();
}

static void update_selected_level() {
    for (int i = 0; i < 7; i++) {
        lv_obj_set_style_bg_color(fan_btns[i], lv_color_hex(0xDDDDDD), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(fan_btns[i], LV_OPA_COVER, LV_PART_MAIN);
    }
    int idx = fan_state.get_current_level() == -1 ? 6 : fan_state.get_current_level();
    lv_obj_set_style_bg_color(fan_btns[idx], lv_color_hex(0x00CC66), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fan_btns[idx], LV_OPA_COVER, LV_PART_MAIN);
}

static void update_power_display() {
    lv_color_t color = fan_state.get_light_power() ? lv_color_hex(0x00CC66) : lv_color_hex(0xFF3333);
    lv_obj_set_style_bg_color(light_btn, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(light_btn, LV_OPA_COVER, LV_PART_MAIN);
}

static void update_timer_display() {
    if (fan_timer.is_running()) {
        unsigned long left = fan_timer.time_left_sec();
        lv_label_set_text_fmt(time_left_label, "%02lu:%02lu:%02lu",
            left / 3600,
            (left % 3600) / 60,
            left % 60);
        lv_obj_clear_flag(time_left_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(time_left_label, "Timer Off");
        lv_obj_add_flag(time_left_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void timer_update_callback() {
    // Called on timer changes (set/cancel)
    update_timer_display();
    // Save the new timer epoch to fan state and persist
    fan_state.set_timer_epoch(fan_timer.get_timer_epoch());
}

static void create_fan_speed_buttons(lv_obj_t* parent) {
    const int radius = 70;
    const lv_coord_t cx = 120;
    const lv_coord_t cy = 120;

    for (int i = 0; i < 7; i++) {
        float angle = ((i - 1) * (360.0 / 6.0)) * (3.1415926535 / 180.0);
        int x = cx + radius * cos(angle);
        int y = cy + radius * sin(angle);

        lv_obj_t* btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_align(btn, LV_ALIGN_CENTER, i == 6 ? 0 : x - cx, i == 6 ? 0 : y - cy);
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, on_fan_speed_btn_clicked, LV_EVENT_CLICKED, NULL);

        lv_obj_t* label = lv_label_create(btn);
        if (i != 6) {
            lv_label_set_text_fmt(label, "%d", i + 1);
        } else {
            lv_label_set_text(label, LV_SYMBOL_STOP);
        }
        lv_obj_center(label);
        fan_btns[i] = btn;
    }
}

lv_obj_t* create_fan_control_page() {
    fan_state.init();
    fan_timer.set_timer_epoch(fan_state.get_timer_epoch());
    fan_timer.set_change_callback(timer_update_callback);

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

    // Light button
    light_btn = lv_btn_create(screen);
    lv_obj_set_size(light_btn, 50, 50);
    lv_obj_align(light_btn, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(light_btn, on_light_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t* light_label = lv_label_create(light_btn);
    lv_label_set_text(light_label, ICONS_LIGHTBULB);
    lv_obj_add_style(light_label, &fa_style, 0);
    lv_obj_center(light_label);
    update_power_display();

    // Reverse direction button
    rev_btn = lv_btn_create(screen);
    lv_obj_set_size(rev_btn, 50, 50);
    lv_obj_align(rev_btn, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    lv_obj_add_event_cb(rev_btn, on_rev_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t* rev_label = lv_label_create(rev_btn);
    lv_label_set_text(rev_label, LV_SYMBOL_REFRESH);
    lv_obj_center(rev_label);

    // Fan speed buttons
    create_fan_speed_buttons(screen);
    update_selected_level();

    // Timer label
    time_left_label = lv_label_create(screen);
    lv_obj_set_style_text_font(time_left_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_left_label, LV_ALIGN_BOTTOM_MID, 0, -25);
    update_timer_display();

    // Timer update every second
    lv_timer_create([](lv_timer_t* t) {
        if (fan_timer.is_running()) {
            update_timer_display();
        }
    }, 1000, nullptr);

    return screen;
}
